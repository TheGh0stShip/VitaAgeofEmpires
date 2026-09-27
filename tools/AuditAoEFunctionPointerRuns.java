// @category AgeOfEmpires

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.mem.MemoryBlock;
import java.io.BufferedWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.HashSet;

/* Read-only audit of contiguous executable-pointer arrays in PE data.
   Runs of four or more entries are candidates, not automatically vtables.
   The output separates absent function starts from interior pointers. */
public class AuditAoEFunctionPointerRuns extends GhidraScript {
    private boolean executablePointer(long value) {
        MemoryBlock target = currentProgram.getMemory().getBlock(toAddr(value));
        return target != null && target.isInitialized() && target.isExecute();
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 1) throw new IllegalArgumentException("OUTPUT_TSV");
        Path output = Path.of(args[0]);
        if (output.getParent() != null) Files.createDirectories(output.getParent());
        int runs = 0;
        HashSet<Long> uniqueMissing = new HashSet<>();
        try (BufferedWriter out = Files.newBufferedWriter(output, StandardCharsets.UTF_8)) {
            out.write("run_start\tentries\tslot\ttarget\tstatus\n");
            for (MemoryBlock block : currentProgram.getMemory().getBlocks()) {
                /* PE resources contain UTF-16 text and icon data whose four-
                   byte groups can accidentally resemble code addresses (for
                   example "ICON" in RoR 1.0a). They cannot own vtables. */
                if (!block.isInitialized() || block.isExecute() ||
                    ".rsrc".equals(block.getName())) continue;
                long start = (block.getStart().getOffset() + 3) & ~3L;
                long end = block.getEnd().getOffset();
                long runStart = 0;
                ArrayList<Long> targets = new ArrayList<>();
                for (long p = start; p + 3 <= end + 4; p += 4) {
                    if (monitor.isCancelled()) break;
                    long value = p + 3 <= end ? Integer.toUnsignedLong(getInt(toAddr(p))) : 0;
                    if (p + 3 <= end && executablePointer(value)) {
                        if (targets.isEmpty()) runStart = p;
                        targets.add(value);
                    } else if (!targets.isEmpty()) {
                        if (targets.size() >= 4) {
                            runs++;
                            for (int i = 0; i < targets.size(); i++) {
                                long target = targets.get(i);
                                Address address = toAddr(target);
                                Function at = getFunctionAt(address);
                                Function containing = at == null ? getFunctionContaining(address) : at;
                                String status = at != null ? "known" :
                                    containing != null ? "inside:" + containing.getEntryPoint() : "missing";
                                if (containing == null) uniqueMissing.add(target);
                                out.write(String.format("%08x\t%d\t%02x\t%08x\t%s\n",
                                    runStart, targets.size(), i * 4, target, status));
                            }
                        }
                        targets.clear();
                    }
                }
            }
        }
        println("AUDITED runs=" + runs + " unique_missing=" + uniqueMissing.size() +
                " output=" + output);
    }
}

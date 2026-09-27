// @category AgeOfEmpires

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.mem.MemoryBlock;
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.TreeSet;

/* Recover the unique missing executable starts from a read-only pointer-run
   audit. Re-check every address against the live program before creating a
   function; never split an existing function or instruction. */
public class RecoverAoEPointerRunFunctions extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 2) throw new IllegalArgumentException("AUDIT_TSV OUTPUT_TSV");
        Path audit = Path.of(args[0]);
        Path output = Path.of(args[1]);
        if (output.getParent() != null) Files.createDirectories(output.getParent());
        TreeSet<Long> candidates = new TreeSet<>();
        try (BufferedReader in = Files.newBufferedReader(audit, StandardCharsets.UTF_8)) {
            String line = in.readLine();
            if (!"run_start\tentries\tslot\ttarget\tstatus".equals(line))
                throw new IllegalArgumentException("Unexpected audit header");
            while ((line = in.readLine()) != null) {
                String[] fields = line.split("\t", -1);
                if (fields.length != 5) throw new IllegalArgumentException("Malformed audit row");
                if (Integer.parseInt(fields[1]) >= 4 && "missing".equals(fields[4]))
                    candidates.add(Long.parseUnsignedLong(fields[3], 16));
            }
        }
        int recovered = 0;
        int skipped = 0;
        try (BufferedWriter out = Files.newBufferedWriter(output, StandardCharsets.UTF_8)) {
            out.write("target\tstatus\n");
            for (long value : candidates) {
                if (monitor.isCancelled()) break;
                Address target = toAddr(value);
                MemoryBlock block = currentProgram.getMemory().getBlock(target);
                String status;
                if (block == null || !block.isInitialized() || !block.isExecute()) {
                    status = "non-executable";
                } else if (getFunctionAt(target) != null) {
                    status = "already-known";
                } else if (getFunctionContaining(target) != null) {
                    status = "inside-function";
                } else {
                    Instruction instruction = getInstructionContaining(target);
                    if (instruction != null && !instruction.getAddress().equals(target)) {
                        status = "inside-instruction";
                    } else {
                        Function function = createFunction(target, null);
                        status = function == null ? "create-failed" : "recovered";
                        if (function != null) recovered++;
                    }
                }
                if (!"recovered".equals(status)) skipped++;
                out.write(String.format("%08x\t%s\n", value, status));
            }
        }
        println("RECOVERED " + recovered + " skipped=" + skipped +
                " candidates=" + candidates.size() + " output=" + output);
    }
}

// @category AgeOfEmpires

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.mem.MemoryBlock;
import java.io.BufferedWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Map;
import java.util.HashMap;
import java.util.TreeMap;
import java.util.TreeSet;

/* Inventory every aligned executable address stored in initialized data,
   including isolated pointers and short tables that the >=4 pointer-run
   audit intentionally omits. This is evidence collection only: pointer-like
   integers are candidates, not proof of function starts. */
public class AuditAoEExecutablePointers extends GhidraScript {
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

        TreeMap<Long, TreeSet<Long>> sourcesByTarget = new TreeMap<>();
        HashMap<Long, String> sourceBlocks = new HashMap<>();
        HashMap<Long, Long> targetBySource = new HashMap<>();
        for (MemoryBlock block : currentProgram.getMemory().getBlocks()) {
            if (!block.isInitialized() || block.isExecute() ||
                ".rsrc".equals(block.getName())) continue;
            long first = (block.getStart().getOffset() + 3) & ~3L;
            long last = block.getEnd().getOffset();
            for (long source = first; source + 3 <= last; source += 4) {
                if (monitor.isCancelled()) break;
                long target = Integer.toUnsignedLong(getInt(toAddr(source)));
                if (!executablePointer(target)) continue;
                sourcesByTarget.computeIfAbsent(target, ignored -> new TreeSet<>())
                    .add(source);
                sourceBlocks.put(source, block.getName());
                targetBySource.put(source, target);
            }
        }

        HashMap<Long, String> shortRunsBySource = new HashMap<>();
        for (MemoryBlock block : currentProgram.getMemory().getBlocks()) {
            if (!block.isInitialized() || block.isExecute() ||
                ".rsrc".equals(block.getName())) continue;
            long first = (block.getStart().getOffset() + 3) & ~3L;
            long last = block.getEnd().getOffset();
            long runStart = 0;
            TreeSet<Long> runSources = new TreeSet<>();
            for (long source = first; source <= last + 4; source += 4) {
                if (targetBySource.containsKey(source)) {
                    if (runSources.isEmpty()) runStart = source;
                    runSources.add(source);
                } else {
                    if (runSources.size() >= 2) {
                        String detail = String.format("%08x:%d", runStart, runSources.size());
                        for (long item : runSources) shortRunsBySource.put(item, detail);
                    }
                    runSources.clear();
                }
            }
        }

        int missing = 0, interior = 0, known = 0;
        try (BufferedWriter out = Files.newBufferedWriter(output,
                StandardCharsets.UTF_8)) {
            out.write("target\tstatus\tinstruction\tinside_instruction\tsource_count\tsources\tblocks\tshort_runs\tnearby_pointer_count\tnearby_sources\n");
            for (Map.Entry<Long, TreeSet<Long>> entry : sourcesByTarget.entrySet()) {
                Address target = toAddr(entry.getKey());
                Function at = getFunctionAt(target);
                Function containing = at == null ? getFunctionContaining(target) : at;
                String status = at != null ? "known" :
                    containing != null ? "inside:" + containing.getEntryPoint() : "missing";
                Instruction instructionAt = getInstructionAt(target);
                Instruction instructionContaining = getInstructionContaining(target);
                String instruction = instructionAt == null ? "" : instructionAt.toString();
                String insideInstruction = instructionContaining == null ? "" :
                    instructionContaining.getAddress().toString();
                if ("known".equals(status)) known++;
                else if (status.startsWith("inside:")) interior++;
                else missing++;
                StringBuilder sources = new StringBuilder();
                StringBuilder blocks = new StringBuilder();
                StringBuilder shortRuns = new StringBuilder();
                TreeMap<Long, Long> nearby = new TreeMap<>();
                for (long source : entry.getValue()) {
                    if (sources.length() != 0) sources.append(',');
                    sources.append(String.format("%08x", source));
                    if (blocks.length() != 0) blocks.append(',');
                    blocks.append(sourceBlocks.get(source));
                    String run = shortRunsBySource.get(source);
                    if (run != null) {
                        if (shortRuns.length() != 0) shortRuns.append(',');
                        shortRuns.append(run);
                    }
                    MemoryBlock sourceBlock = currentProgram.getMemory().getBlock(toAddr(source));
                    for (long nearbySource = source - 32; nearbySource <= source + 32; nearbySource += 4) {
                        if (nearbySource == source || !targetBySource.containsKey(nearbySource)) continue;
                        MemoryBlock nearbyBlock = currentProgram.getMemory()
                            .getBlock(toAddr(nearbySource));
                        if (sourceBlock != null && nearbyBlock != null &&
                            sourceBlock.getStart().equals(nearbyBlock.getStart()))
                            nearby.put(nearbySource, targetBySource.get(nearbySource));
                    }
                }
                StringBuilder nearbySources = new StringBuilder();
                for (Map.Entry<Long, Long> neighbor : nearby.entrySet()) {
                    if (nearbySources.length() != 0) nearbySources.append(',');
                    nearbySources.append(String.format("%08x=>%08x",
                        neighbor.getKey(), neighbor.getValue()));
                }
                out.write(String.format("%08x\t%s\t%s\t%s\t%d\t%s\t%s\t%s\t%d\t%s\n", entry.getKey(),
                    status, instruction, insideInstruction, entry.getValue().size(),
                    sources, blocks, shortRuns, nearby.size(), nearbySources));
            }
        }
        println("DONE targets=" + sourcesByTarget.size() + " missing=" + missing +
            " interior=" + interior + " known=" + known +
            " cancelled=" + monitor.isCancelled() + " output=" + output);
    }
}

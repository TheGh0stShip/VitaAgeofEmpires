// @category AgeOfEmpires

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.CodeUnit;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.lang.OperandType;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.Reference;
import java.io.BufferedWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Map;
import java.util.TreeSet;
import java.util.TreeMap;

/* Read-only inventory of aligned executable-to-executable pointer values.
   This specifically catches code/data tables embedded in execute sections,
   which the initialized-non-executable pointer audit omits. */
public class AuditAoEEmbeddedExecutablePointers extends GhidraScript {
    private static class Candidate {
        final long source;
        final long target;
        final String block;
        Candidate(long source, long target, String block) {
            this.source = source;
            this.target = target;
            this.block = block;
        }
    }

    private boolean executable(long value) {
        MemoryBlock block = currentProgram.getMemory().getBlock(toAddr(value));
        return block != null && block.isInitialized() && block.isExecute();
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 1) throw new IllegalArgumentException("OUTPUT_TSV");
        Path output = Path.of(args[0]);
        if (output.getParent() != null) Files.createDirectories(output.getParent());

        TreeMap<Long, Candidate> candidates = new TreeMap<>();
        TreeSet<Long> indirectJumpTableBases = new TreeSet<>();
        InstructionIterator instructions = currentProgram.getListing().getInstructions(true);
        while (instructions.hasNext() && !monitor.isCancelled()) {
            Instruction instruction = instructions.next();
            if (!instruction.getFlowType().isJump() ||
                !instruction.getFlowType().isComputed()) continue;
            for (int operand = 0; operand < instruction.getNumOperands(); operand++) {
                if (!OperandType.isAddress(instruction.getOperandType(operand))) continue;
                Object[] values = instruction.getOpObjects(operand);
                for (Object value : values) {
                    if (value instanceof Address) {
                        indirectJumpTableBases.add(((Address)value).getOffset());
                    }
                }
            }
        }
        for (MemoryBlock block : currentProgram.getMemory().getBlocks()) {
            if (!block.isInitialized() || !block.isExecute() ||
                ".rsrc".equals(block.getName())) continue;
            long first = (block.getStart().getOffset() + 3) & ~3L;
            long last = block.getEnd().getOffset();
            for (long source = first; source + 3 <= last; source += 4) {
                if (monitor.isCancelled()) break;
                long target = Integer.toUnsignedLong(getInt(toAddr(source)));
                if (!executable(target)) continue;
                candidates.put(source, new Candidate(source, target, block.getName()));
            }
        }

        int emitted = 0, withExactReference = 0, inRuns = 0;
        try (BufferedWriter out = Files.newBufferedWriter(output,
                StandardCharsets.UTF_8)) {
            out.write("source\ttarget\tstatus\tsource_block\tsource_unit\tsource_function\tinstruction\texact_reference\tnearby_count\tnearby_targets\tjump_table_base\n");
            for (Candidate candidate : candidates.values()) {
                Address source = toAddr(candidate.source);
                Address target = toAddr(candidate.target);
                boolean exact = false;
                StringBuilder refs = new StringBuilder();
                for (Reference ref : currentProgram.getReferenceManager()
                        .getReferencesFrom(source)) {
                    if (!target.equals(ref.getToAddress())) continue;
                    exact = true;
                    if (refs.length() != 0) refs.append(',');
                    refs.append(ref.getReferenceType());
                }

                TreeMap<Long, Long> nearby = new TreeMap<>();
                MemoryBlock sourceBlock = currentProgram.getMemory().getBlock(source);
                for (long neighbor = candidate.source - 32;
                     neighbor <= candidate.source + 32; neighbor += 4) {
                    if (neighbor == candidate.source || !candidates.containsKey(neighbor)) continue;
                    MemoryBlock neighborBlock = currentProgram.getMemory()
                        .getBlock(toAddr(neighbor));
                    if (sourceBlock != null && neighborBlock != null &&
                        sourceBlock.getStart().equals(neighborBlock.getStart()))
                        nearby.put(neighbor, candidates.get(neighbor).target);
                }
                if (!exact && nearby.size() < 2) continue;

                CodeUnit unit = currentProgram.getListing().getCodeUnitContaining(source);
                Function function = getFunctionContaining(source);
                Instruction instruction = getInstructionContaining(source);
                StringBuilder targets = new StringBuilder();
                for (Map.Entry<Long, Long> entry : nearby.entrySet()) {
                    if (targets.length() != 0) targets.append(',');
                    targets.append(String.format("%08x=>%08x", entry.getKey(), entry.getValue()));
                }
                boolean jumpTableEntry = false;
                for (long base : indirectJumpTableBases) {
                    if (candidate.source >= base && candidate.source < base + 256) {
                        jumpTableEntry = true;
                        break;
                    }
                }
                Function targetFunction = getFunctionAt(target);
                Function targetContaining = targetFunction == null ?
                    getFunctionContaining(target) : targetFunction;
                String status = targetFunction != null ? "known" :
                    targetContaining != null ? "inside:" + targetContaining.getEntryPoint() :
                    "missing";
                out.write(String.format("%08x\t%08x\t%s\t%s\t%s\t%s\t%s\t%s\t%d\t%s\t%s\n",
                    candidate.source, candidate.target, status, candidate.block,
                    unit == null ? "" : unit.getClass().getSimpleName(),
                    function == null ? "" : function.getEntryPoint().toString(),
                    instruction == null ? "" : instruction.toString(), refs,
                    nearby.size(), targets, jumpTableEntry ? "yes" : ""));
                emitted++;
                if (exact) withExactReference++;
                if (nearby.size() >= 2) inRuns++;
            }
        }
        println("DONE aligned_exec_pointer_values=" + candidates.size() +
            " emitted=" + emitted + " exact_references=" + withExactReference +
            " in_pointer_runs=" + inRuns + " cancelled=" + monitor.isCancelled() +
            " output=" + output);
    }
}

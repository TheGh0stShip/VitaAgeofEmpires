// @category AgeOfEmpires

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.listing.CodeUnit;
import ghidra.program.model.listing.Data;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.listing.Listing;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.Reference;
import java.io.BufferedWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

/* Inventory executable memory by listing category, function membership, and
   undefined gaps. Gaps are triage candidates, never automatically functions. */
public class AuditAoEExecutableCoverage extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 2)
            throw new IllegalArgumentException("OUTPUT_SUMMARY OUTPUT_GAPS");
        Path summary = Path.of(args[0]);
        Path gaps = Path.of(args[1]);
        if (summary.equals(gaps)) throw new IllegalArgumentException("Outputs must differ");
        if (summary.getParent() != null) Files.createDirectories(summary.getParent());
        if (gaps.getParent() != null) Files.createDirectories(gaps.getParent());

        long executableBytes = 0, instructionBytes = 0, functionBytes = 0;
        long instructionCount = 0, definedDataBytes = 0, undefinedBytes = 0;
        int blocks = 0, undefinedSpans = 0, outsideFunctionSpans = 0;
        Listing listing = currentProgram.getListing();
        try (BufferedWriter summaryOut = Files.newBufferedWriter(summary,
                     StandardCharsets.UTF_8);
             BufferedWriter gapsOut = Files.newBufferedWriter(gaps,
                     StandardCharsets.UTF_8)) {
            summaryOut.write("block\tstart\tend\texecutable_bytes\tinstruction_bytes\tfunction_bytes\tinstructions\tdefined_data_bytes\tundefined_bytes\tundefined_spans\tdecoded_outside_function_spans\n");
            gapsOut.write("block\tstart\tend\tbytes\tinstruction_count\tfamily\tincoming_references\tflow_exits\tclassification\n");
            for (MemoryBlock block : currentProgram.getMemory().getBlocks()) {
                if (!block.isInitialized() || !block.isExecute()) continue;
                blocks++;
                long blockExec = block.getSize(), blockInsn = 0, blockFunc = 0;
                long blockInstructions = 0, blockData = 0, blockUndefined = 0;
                int blockUndefinedSpans = 0, blockOutsideSpans = 0;
                Address islandStart = null, islandEnd = null;
                int islandInstructions = 0;
                String islandFamily = "";
                StringBuilder islandRefs = new StringBuilder();
                StringBuilder islandExits = new StringBuilder();
                AddressSet set = new AddressSet(block.getStart(), block.getEnd());
                InstructionIterator it = listing.getInstructions(set, true);
                while (it.hasNext() && !monitor.isCancelled()) {
                    Instruction instruction = it.next();
                    long size = instruction.getLength();
                    blockInsn += size;
                    blockInstructions++;
                    Function function = currentProgram.getFunctionManager()
                        .getFunctionContaining(instruction.getAddress());
                    if (function != null) {
                        blockFunc += size;
                        if (islandStart != null) {
                            writeIsland(gapsOut, block, islandStart, islandEnd,
                                islandInstructions, islandFamily, islandRefs, islandExits);
                            blockOutsideSpans++;
                            islandStart = null;
                            islandInstructions = 0;
                            islandFamily = "";
                            islandRefs.setLength(0);
                            islandExits.setLength(0);
                        }
                    } else {
                        if (islandStart == null) {
                            islandStart = instruction.getAddress();
                            islandFamily = instruction.getMnemonicString();
                        }
                        islandEnd = instruction.getMaxAddress();
                        islandInstructions++;
                        for (Reference reference : currentProgram.getReferenceManager()
                                .getReferencesTo(instruction.getAddress())) {
                            appendUnique(islandRefs, reference.getFromAddress().toString());
                        }
                        if (!instruction.getFlowType().hasFallthrough() ||
                            instruction.getFlowType().isJump() || instruction.getFlowType().isCall())
                            appendUnique(islandExits, instruction.getAddress() + ":" +
                                instruction.getFlowType().toString());
                    }
                }
                if (islandStart != null) {
                    writeIsland(gapsOut, block, islandStart, islandEnd,
                        islandInstructions, islandFamily, islandRefs, islandExits);
                    blockOutsideSpans++;
                }

                Address cursor = block.getStart();
                Instruction previous = null;
                while (cursor != null && cursor.compareTo(block.getEnd()) <= 0 &&
                       !monitor.isCancelled()) {
                    CodeUnit unit = listing.getCodeUnitAt(cursor);
                    if (unit != null && unit.getMinAddress().equals(cursor)) {
                        if (unit instanceof Instruction) {
                            previous = (Instruction) unit;
                        } else if (unit instanceof Data) {
                            blockData += unit.getLength();
                        }
                        cursor = unit.getMaxAddress().next();
                        continue;
                    }

                    Address start = cursor;
                    CodeUnit after = listing.getCodeUnitAfter(cursor);
                    Address nextDefined = after == null ||
                        after.getMinAddress().compareTo(block.getEnd()) > 0 ? null :
                        after.getMinAddress();
                    Address end = nextDefined == null || nextDefined.compareTo(block.getEnd()) > 0
                        ? block.getEnd() : nextDefined.previous();
                    if (end.compareTo(start) < 0) {
                        cursor = cursor.next();
                        continue;
                    }
                    long size = end.subtract(start) + 1;
                    Instruction nextInstruction = null;
                    if (nextDefined != null && nextDefined.compareTo(block.getEnd()) <= 0) {
                        CodeUnit nextUnit = listing.getCodeUnitAt(nextDefined);
                        if (nextUnit instanceof Instruction) nextInstruction = (Instruction) nextUnit;
                    }
                    blockUndefined += size;
                    blockUndefinedSpans++;
                    String classification = size >= 5 ? "undefined_candidate_ge5" : "undefined_short";
                    gapsOut.write(block.getName() + "\t" + start + "\t" + end +
                        "\t" + size + "\t" +
                        (previous == null ? "" : previous.getAddress().toString()) + "\t" +
                        (nextInstruction == null ? "" : nextInstruction.getAddress().toString()) +
                        "\t" + classification + "\n");
                    cursor = end.next();
                }
                summaryOut.write(block.getName() + "\t" + block.getStart() + "\t" +
                    block.getEnd() + "\t" + blockExec + "\t" + blockInsn + "\t" +
                    blockFunc + "\t" + blockInstructions + "\t" + blockData + "\t" +
                    blockUndefined + "\t" + blockUndefinedSpans + "\t" +
                    blockOutsideSpans + "\n");
                executableBytes += blockExec;
                instructionBytes += blockInsn;
                functionBytes += blockFunc;
                instructionCount += blockInstructions;
                definedDataBytes += blockData;
                undefinedBytes += blockUndefined;
                undefinedSpans += blockUndefinedSpans;
                outsideFunctionSpans += blockOutsideSpans;
            }
        }
        println("DONE blocks=" + blocks + " executable_bytes=" + executableBytes +
            " instruction_bytes=" + instructionBytes + " function_instruction_bytes=" +
            functionBytes + " instructions=" + instructionCount + " defined_data_bytes=" +
            definedDataBytes + " undefined_bytes=" + undefinedBytes +
            " undefined_spans=" + undefinedSpans + " decoded_outside_function_islands=" +
            outsideFunctionSpans + " cancelled=" + monitor.isCancelled());
    }

    private void appendUnique(StringBuilder list, String value) {
        String token = "|" + value + "|";
        if (list.indexOf(token) >= 0) return;
        if (list.length() != 0) list.append(',');
        list.append(value);
    }

    private void writeIsland(BufferedWriter out, MemoryBlock block, Address start,
            Address end, int instructionCount, String family, StringBuilder refs,
            StringBuilder exits) throws Exception {
        out.write(block.getName() + "\t" + start + "\t" + end + "\t" +
            (end.subtract(start) + 1) + "\t" + instructionCount + "\t" + family +
            "\t" + refs + "\t" + exits + "\tdecoded_outside_function\n");
    }
}

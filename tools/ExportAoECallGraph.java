// @category AgeOfEmpires

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.symbol.Reference;
import java.io.BufferedWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

/* Whole-program, read-only call-site inventory. Direct references preserve
   their exact call-site address; calls without a resolved call reference are
   marked indirect_or_unresolved. This is an analysis map, not source code. */
public class ExportAoECallGraph extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 1)
            throw new IllegalArgumentException("OUTPUT_TSV");
        Path output = Path.of(args[0]);
        if (output.getParent() != null) Files.createDirectories(output.getParent());
        int functions = 0, sites = 0, direct = 0, unresolved = 0;
        try (BufferedWriter writer = Files.newBufferedWriter(output,
                     StandardCharsets.UTF_8)) {
            writer.write("caller\tcall_site\tcallee\tcallee_name\tkind\n");
            FunctionIterator all = currentProgram.getFunctionManager()
                .getFunctions(true);
            while (all.hasNext() && !monitor.isCancelled()) {
                Function caller = all.next();
                if (caller.isExternal()) continue;
                functions++;
                InstructionIterator instructions = currentProgram.getListing()
                    .getInstructions(caller.getBody(), true);
                while (instructions.hasNext() && !monitor.isCancelled()) {
                    Instruction instruction = instructions.next();
                    if (!instruction.getFlowType().isCall()) continue;
                    sites++;
                    Address site = instruction.getAddress();
                    boolean found = false;
                    for (Reference reference : instruction.getReferencesFrom()) {
                        if (!reference.getReferenceType().isCall()) continue;
                        Address targetAddress = reference.getToAddress();
                        Function target = currentProgram.getFunctionManager()
                            .getFunctionAt(targetAddress);
                        if (target == null)
                            target = currentProgram.getFunctionManager()
                                .getFunctionContaining(targetAddress);
                        writer.write(caller.getEntryPoint() + "\t" + site + "\t" +
                                     targetAddress + "\t" +
                                     (target == null ? "" : target.getName()) + "\t" +
                                     (target == null ? "unmapped_direct" :
                                      target.isExternal() ? "external" : "internal") +
                                     "\n");
                        direct++;
                        found = true;
                    }
                    if (!found) {
                        writer.write(caller.getEntryPoint() + "\t" + site +
                                     "\t\t\tindirect_or_unresolved\n");
                        unresolved++;
                    }
                }
                if (functions % 500 == 0) {
                    writer.flush();
                    println("SCANNED " + functions + " functions; " + sites +
                            " call sites");
                }
            }
        }
        println("DONE functions=" + functions + " sites=" + sites +
                " direct_references=" + direct + " unresolved_sites=" +
                unresolved + " cancelled=" + monitor.isCancelled());
    }
}

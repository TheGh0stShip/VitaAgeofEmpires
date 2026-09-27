// @category AgeOfEmpires

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.symbol.Reference;
import java.io.BufferedWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

/* Promote manually triaged executable instruction-island entries. Candidate
   starts must be supplied from the byte-coverage TSV/raw-disassembly audit;
   this script never guesses starts from arbitrary code bytes. */
public class RecoverAoEExecutableIslandFunctions extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 2)
            throw new IllegalArgumentException("OUTPUT_TSV ADDRESS [ADDRESS ...]");
        Path output = Path.of(args[0]);
        if (output.getParent() != null) Files.createDirectories(output.getParent());
        int recovered = 0, skipped = 0, noInstruction = 0;
        try (BufferedWriter out = Files.newBufferedWriter(output,
                     StandardCharsets.UTF_8)) {
            out.write("address\tstatus\tinstruction\treferences\tfunction\tend\n");
            for (int i = 1; i < args.length; i++) {
                Address address = toAddr(args[i]);
                Instruction instruction = getInstructionAt(address);
                StringBuilder refs = new StringBuilder();
                for (Reference ref : currentProgram.getReferenceManager()
                        .getReferencesTo(address)) {
                    if (refs.length() != 0) refs.append(',');
                    refs.append(ref.getFromAddress()).append(':')
                        .append(ref.getReferenceType());
                }
                Function function = getFunctionAt(address);
                String status;
                if (instruction == null) {
                    status = "not_instruction_boundary";
                    noInstruction++;
                } else if (function != null) {
                    status = "already_function";
                    skipped++;
                } else if (getFunctionContaining(address) != null) {
                    status = "inside_existing_function";
                    skipped++;
                } else {
                    function = createFunction(address, null);
                    status = function == null ? "create_failed" : "recovered";
                    if (function == null) skipped++;
                    else recovered++;
                }
                out.write(address + "\t" + status + "\t" +
                    (instruction == null ? "" : instruction.toString()) + "\t" + refs + "\t" +
                    (function == null ? "" : function.getName()) + "\t" +
                    (function == null ? "" : function.getBody().getMaxAddress()) + "\n");
                println(address + " " + status + " refs=" + refs);
            }
        }
        println("DONE recovered=" + recovered + " skipped=" + skipped +
            " no_instruction=" + noInstruction + " cancelled=" + monitor.isCancelled());
    }
}

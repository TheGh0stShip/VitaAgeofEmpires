// @category AgeOfEmpires

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.TreeSet;

/* Recover missing starts only with multiple-pointer table evidence: either
   a contiguous executable-pointer run or at least two nearby pointers in the
   same data block. Never split an existing function/instruction. */
public class RecoverAoEExecutablePointerFunctions extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 2) throw new IllegalArgumentException("AUDIT_TSV OUTPUT_TSV");
        Path audit = Path.of(args[0]);
        Path output = Path.of(args[1]);
        if (output.getParent() != null) Files.createDirectories(output.getParent());

        TreeSet<Long> candidates = new TreeSet<>();
        try (BufferedReader in = Files.newBufferedReader(audit, StandardCharsets.UTF_8)) {
            String header = in.readLine();
            if (!"target\tstatus\tinstruction\tinside_instruction\tsource_count\tsources\tblocks\tshort_runs\tnearby_pointer_count\tnearby_sources".equals(header))
                throw new IllegalArgumentException("Unexpected audit header");
            String line;
            while ((line = in.readLine()) != null) {
                String[] fields = line.split("\t", -1);
                if (fields.length != 10) throw new IllegalArgumentException("Malformed audit row");
                if (!"missing".equals(fields[1]) ||
                    (fields[7].isEmpty() && Integer.parseInt(fields[8]) < 2)) continue;
                candidates.add(Long.parseUnsignedLong(fields[0], 16));
            }
        }

        int recovered = 0, skipped = 0;
        try (BufferedWriter out = Files.newBufferedWriter(output, StandardCharsets.UTF_8)) {
            out.write("target\tstatus\tinstruction\n");
            for (long value : candidates) {
                if (monitor.isCancelled()) break;
                Address target = toAddr(value);
                Function at = getFunctionAt(target);
                Function containing = getFunctionContaining(target);
                Instruction instruction = getInstructionContaining(target);
                String status;
                if (at != null) {
                    status = "already-known";
                } else if (containing != null) {
                    status = "inside-function:" + containing.getEntryPoint();
                } else if (instruction != null && !instruction.getAddress().equals(target)) {
                    status = "inside-instruction:" + instruction.getAddress();
                } else {
                    Function function = createFunction(target, null);
                    status = function == null ? "create-failed" : "recovered";
                    if (function != null) recovered++;
                }
                if (!"recovered".equals(status)) skipped++;
                Instruction atTarget = getInstructionAt(target);
                out.write(String.format("%08x\t%s\t%s\n", value, status,
                    atTarget == null ? "" : atTarget.toString()));
            }
        }
        println("RECOVERED " + recovered + " skipped=" + skipped +
            " candidates=" + candidates.size() + " cancelled=" + monitor.isCancelled() +
            " output=" + output);
    }
}

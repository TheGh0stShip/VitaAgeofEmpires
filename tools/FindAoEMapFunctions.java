// @category AgeOfEmpires

import ghidra.app.script.GhidraScript;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.scalar.Scalar;
import java.util.Map;
import java.util.TreeMap;

/* Index functions touching the original map object's row-pointer/terrain
   fields. This identifies drawing candidates without decompiling the whole
   executable or assuming nearby addresses contain the renderer. */
public class FindAoEMapFunctions extends GhidraScript {
    @Override
    public void run() throws Exception {
        long[] targets = {0x8d8cL, 0x8d90L, 0x8d92L, 0x8d9aL,
                          0x8d9cL, 0x8d9eL, 0x8dc0L};
        Map<String, StringBuilder> hits = new TreeMap<>();
        InstructionIterator instructions = currentProgram.getListing().getInstructions(true);
        while (instructions.hasNext() && !monitor.isCancelled()) {
            Instruction instruction = instructions.next();
            for (int operand = 0; operand < instruction.getNumOperands(); operand++) {
                for (Object object : instruction.getOpObjects(operand)) {
                    if (!(object instanceof Scalar)) continue;
                    long value = ((Scalar)object).getUnsignedValue();
                    boolean matched = false;
                    for (long target : targets) if (value == target) matched = true;
                    if (!matched) continue;
                    Function function = getFunctionContaining(instruction.getAddress());
                    if (function == null) continue;
                    String key = function.getEntryPoint().toString();
                    hits.computeIfAbsent(key, k -> new StringBuilder())
                        .append(String.format(" %x@%s", value, instruction.getAddress()));
                }
            }
        }
        for (Map.Entry<String, StringBuilder> hit : hits.entrySet())
            println(hit.getKey() + hit.getValue());
    }
}

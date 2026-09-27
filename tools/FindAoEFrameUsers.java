// @category AgeOfEmpires

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import java.util.Map;
import java.util.TreeMap;

/* Candidate locator for terrain DAT frame-table consumers. In the 1997
   executable, terrain records are 0x198 bytes and the first elevation
   frame triple begins at map-object offset 0xd2. */
public class FindAoEFrameUsers extends GhidraScript {
    @Override
    public void run() throws Exception {
        Map<String, int[]> hits = new TreeMap<>();
        InstructionIterator instructions = currentProgram.getListing().getInstructions(true);
        while (instructions.hasNext() && !monitor.isCancelled()) {
            Instruction instruction = instructions.next();
            String value = instruction.toString().toLowerCase();
            int bit = 0;
            if (value.contains("0xd2")) bit |= 1;
            if (value.contains("0x198")) bit |= 2;
            if (value.contains("0x5a0")) bit |= 4;
            if (value.contains("0x8d8c")) bit |= 8;
            if (bit == 0) continue;
            Function function = getFunctionContaining(instruction.getAddress());
            if (function == null) continue;
            int[] counts = hits.computeIfAbsent(function.getEntryPoint().toString(),
                                                key -> new int[4]);
            for (int i = 0; i < 4; ++i) if ((bit & (1 << i)) != 0) counts[i]++;
        }
        for (Map.Entry<String, int[]> hit : hits.entrySet()) {
            int[] n = hit.getValue();
            if ((n[0] > 0 && n[1] > 0) || (n[0] > 1 && n[3] > 0))
                println(String.format("CANDIDATE %s frame=%d terrain_stride=%d " +
                                      "border_stride=%d map_rows=%d",
                                      hit.getKey(), n[0], n[1], n[2], n[3]));
        }
    }
}

// @category AgeOfEmpires

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.Reference;

/* The application's frame loop calls the active game object's virtual
   method at +0x54. Enumerate data vtables with executable +0x20, +0x48,
   and +0x54 entries and a constructor/code reference to their base. */
public class FindAoEFrameVtables extends GhidraScript {
    private Function slot(Address table, int offset) throws Exception {
        long value = Integer.toUnsignedLong(getInt(table.add(offset)));
        return getFunctionAt(toAddr(value));
    }

    @Override
    public void run() throws Exception {
        for (MemoryBlock block : currentProgram.getMemory().getBlocks()) {
            if (!block.isInitialized() || block.isExecute()) continue;
            long end = block.getEnd().getOffset();
            for (long p = block.getStart().getOffset(); p + 0x57 <= end;
                 p = (p + 4) & ~3L) {
                if (monitor.isCancelled()) return;
                Address table = toAddr(p);
                int codeRefs = 0;
                for (Reference ref : currentProgram.getReferenceManager()
                                            .getReferencesTo(table)) {
                    MemoryBlock source = currentProgram.getMemory()
                                                       .getBlock(ref.getFromAddress());
                    if (source != null && source.isExecute()) codeRefs++;
                }
                if (codeRefs == 0) continue;
                Function state = slot(table, 0x20);
                Function input = slot(table, 0x48);
                Function frame = slot(table, 0x54);
                if (state == null || input == null || frame == null) continue;
                println(String.format("VTABLE %08x refs=%d state=%s input=%s frame=%s",
                        p, codeRefs, state.getEntryPoint(),
                        input.getEntryPoint(), frame.getEntryPoint()));
            }
        }
    }
}

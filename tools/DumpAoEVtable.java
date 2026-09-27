// @category AgeOfEmpires

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;

public class DumpAoEVtable extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 2) throw new IllegalArgumentException("VTABLE_ADDRESS SLOT_COUNT");
        Address start = toAddr(args[0]);
        int count = Integer.parseInt(args[1]);
        if (count < 1 || count > 256) throw new IllegalArgumentException("slot count");
        for (int i = 0; i < count; ++i) {
            long pointer = Integer.toUnsignedLong(getInt(start.add(i * 4L)));
            Address target = toAddr(pointer);
            Function function = getFunctionAt(target);
            println(String.format("SLOT %02x %08x %s", i * 4, pointer,
                                  function == null ? "" : function.getName()));
        }
    }
}

// @category AgeOfEmpires

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.Reference;

public class DumpAoEFunctions extends GhidraScript {
    @Override
    public void run() throws Exception {
        DecompInterface decompiler = new DecompInterface();
        decompiler.openProgram(currentProgram);
        try {
            for (String textAddress : getScriptArgs()) {
                Address address = toAddr(textAddress);
                for (Reference reference : currentProgram.getReferenceManager().getReferencesTo(address)) {
                    println("REFERENCE TO " + textAddress + " FROM " + reference.getFromAddress());
                }
                Function function = getFunctionContaining(address);
                if (function == null) {
                    function = createFunction(address, null);
                    if (function == null) {
                        println("No function contains " + textAddress);
                        continue;
                    }
                }
                DecompileResults result = decompiler.decompileFunction(function, 60, monitor);
                println("FUNCTION " + function.getName() + " AT " + function.getEntryPoint());
                if (result.decompileCompleted()) {
                    println(result.getDecompiledFunction().getC());
                } else {
                    println("Decompilation failed: " + result.getErrorMessage());
                }
            }
        } finally {
            decompiler.dispose();
        }
    }
}

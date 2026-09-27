// @category AgeOfEmpires

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.Reference;

public class FindAoEStringReferences extends GhidraScript {
    @Override
    public void run() throws Exception {
        for (String argument : getScriptArgs()) {
            Address address = toAddr(argument);
            for (Reference reference : currentProgram.getReferenceManager().getReferencesTo(address)) {
                Function function = getFunctionContaining(reference.getFromAddress());
                println(argument + " FROM " + reference.getFromAddress() +
                        " IN " + (function == null ? "<unknown>" : function.getEntryPoint()));
            }
        }
    }
}

// @category AgeOfEmpires

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.mem.MemoryBlock;
import java.io.BufferedWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

/* Recover executable function starts named by an original PE vtable, then
   export their pseudocode. This complements, rather than replaces, the bulk
   export: the analyzer can leave valid virtual methods unidentified. */
public class RecoverAoEVtableFunctions extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 3)
            throw new IllegalArgumentException("VTABLE_ADDRESS SLOT_COUNT OUTPUT_TEXT");
        Address table = toAddr(args[0]);
        int count = Integer.parseInt(args[1]);
        if (count < 1 || count > 256) throw new IllegalArgumentException("slot count");
        Path output = Path.of(args[2]);
        if (output.getParent() != null) Files.createDirectories(output.getParent());

        DecompInterface decompiler = new DecompInterface();
        if (!decompiler.openProgram(currentProgram))
            throw new IllegalStateException("Cannot open program in decompiler");
        int recovered = 0;
        try (BufferedWriter out = Files.newBufferedWriter(output, StandardCharsets.UTF_8)) {
            out.write("/* Vtable-derived Ghidra reference export. NOT buildable source. */\n\n");
            for (int i = 0; i < count && !monitor.isCancelled(); i++) {
                long pointer = Integer.toUnsignedLong(getInt(table.add(i * 4L)));
                Address target = toAddr(pointer);
                MemoryBlock block = currentProgram.getMemory().getBlock(target);
                String status;
                Function function = null;
                if (block == null || !block.isExecute()) {
                    status = "non-executable";
                } else {
                    function = getFunctionAt(target);
                    if (function == null) {
                        Function containing = getFunctionContaining(target);
                        if (containing != null) {
                            status = "inside " + containing.getEntryPoint();
                        } else {
                            function = createFunction(target, null);
                            status = function == null ? "create-failed" : "recovered";
                            if (function != null) recovered++;
                        }
                    } else {
                        status = "known";
                    }
                }
                out.write(String.format("/* SLOT %02x TARGET %08x %s */\n", i * 4, pointer, status));
                if (function != null) {
                    DecompileResults result = decompiler.decompileFunction(function, 60, monitor);
                    if (result.decompileCompleted() && result.getDecompiledFunction() != null)
                        out.write(result.getDecompiledFunction().getC());
                    else
                        out.write("/* Decompilation failed: " + result.getErrorMessage() + " */\n");
                }
                out.write("\n\n");
            }
        } finally {
            decompiler.dispose();
        }
        println("RECOVERED " + recovered + " vtable function starts to " + output);
    }
}

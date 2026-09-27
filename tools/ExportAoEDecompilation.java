// @category AgeOfEmpires

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import java.io.BufferedWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

/* Bulk reference export of every function discovered in one analyzed PE.
   The output is decompiler evidence, not a buildable source port. Invoke
   through analyzeHeadless with OUTPUT_TEXT OUTPUT_INDEX. */
public class ExportAoEDecompilation extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 2)
            throw new IllegalArgumentException("OUTPUT_TEXT OUTPUT_INDEX");
        Path textPath = Path.of(args[0]);
        Path indexPath = Path.of(args[1]);
        if (textPath.equals(indexPath))
            throw new IllegalArgumentException("Output paths must differ");
        if (textPath.getParent() != null) Files.createDirectories(textPath.getParent());
        if (indexPath.getParent() != null) Files.createDirectories(indexPath.getParent());

        DecompInterface decompiler = new DecompInterface();
        if (!decompiler.openProgram(currentProgram))
            throw new IllegalStateException("Cannot open program in decompiler");
        int total = 0;
        int complete = 0;
        int failed = 0;
        try (BufferedWriter source = Files.newBufferedWriter(textPath,
                     StandardCharsets.UTF_8);
             BufferedWriter index = Files.newBufferedWriter(indexPath,
                     StandardCharsets.UTF_8)) {
            source.write("/* Ghidra reference export: " + currentProgram.getName() +
                         ". NOT buildable source. */\n\n");
            index.write("address\tname\tstatus\tline\n");
            long line = 3;
            FunctionIterator functions = currentProgram.getFunctionManager()
                .getFunctions(true);
            while (functions.hasNext() && !monitor.isCancelled()) {
                Function function = functions.next();
                total++;
                String address = function.getEntryPoint().toString();
                String name = function.getName();
                String status;
                String body;
                if (function.isExternal()) {
                    status = "external";
                    body = "/* External function; no body in this binary. */\n";
                } else {
                    // The original hand-optimized pixel blitters contain
                    // large computed-jump bodies; 30 seconds times out on
                    // them even though a 60-second targeted pass succeeds.
                    DecompileResults result = decompiler.decompileFunction(
                        function, 60, monitor);
                    if (result.decompileCompleted() &&
                        result.getDecompiledFunction() != null) {
                        status = "ok";
                        body = result.getDecompiledFunction().getC();
                        complete++;
                    } else {
                        status = "failed";
                        String reason = result.getErrorMessage();
                        body = "/* Decompilation failed: " +
                               (reason == null ? "unknown" : reason.replace("*/", "* /")) +
                               " */\n";
                        failed++;
                    }
                }
                source.write("/* FUNCTION " + address + " " + name + " */\n");
                source.write(body);
                source.write("\n\n");
                index.write(address + "\t" + name + "\t" + status +
                            "\t" + line + "\n");
                line += 1 + body.lines().count() + 2;
                if (total % 200 == 0) {
                    source.flush();
                    index.flush();
                    println("EXPORTED " + total + " functions, " + complete +
                            " decompiled, " + failed + " failed");
                }
            }
        } finally {
            decompiler.dispose();
        }
        println("DONE " + total + " functions, " + complete +
                " decompiled, " + failed + " failed; cancelled=" +
                monitor.isCancelled());
    }
}

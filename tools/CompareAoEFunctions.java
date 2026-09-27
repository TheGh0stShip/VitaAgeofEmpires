// @category AgeOfEmpires

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.PriorityQueue;

/* Finds candidate matches between the base and expansion executables using
   instruction-mnemonic trigrams. This is only a locator; each candidate
   still needs decompilation and data/behavior checks. */
public class CompareAoEFunctions extends GhidraScript {
    private List<String> mnemonics(Function function) {
        List<String> result = new ArrayList<>();
        InstructionIterator instructions = currentProgram.getListing()
            .getInstructions(function.getBody(), true);
        while (instructions.hasNext()) {
            Instruction instruction = instructions.next();
            String name = instruction.getMnemonicString().toUpperCase();
            if (!name.equals("NOP")) result.add(name);
        }
        return result;
    }

    private Map<String, Integer> trigrams(List<String> sequence) {
        Map<String, Integer> result = new HashMap<>();
        for (int i = 2; i < sequence.size(); ++i) {
            String key = sequence.get(i - 2) + "," + sequence.get(i - 1) +
                         "," + sequence.get(i);
            result.merge(key, 1, Integer::sum);
        }
        return result;
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 2) throw new IllegalArgumentException("emit ADDRESS... | search SIGNATURE");
        if (args[0].equals("emit")) {
            for (int i = 1; i < args.length; ++i) {
                Function function = getFunctionContaining(toAddr(args[i]));
                if (function == null) throw new IllegalArgumentException("No function at " + args[i]);
                println("SIGNATURE " + function.getEntryPoint() + " " +
                        String.join(",", mnemonics(function)));
            }
            return;
        }
        if (!args[0].equals("search")) throw new IllegalArgumentException(args[0]);
        List<List<String>> targets = new ArrayList<>();
        List<Map<String, Integer>> references = new ArrayList<>();
        List<PriorityQueue<Match>> bests = new ArrayList<>();
        for (int i = 1; i < args.length; ++i) {
            List<String> target = List.of(args[i].split(","));
            targets.add(target);
            references.add(trigrams(target));
            bests.add(new PriorityQueue<>((a, b) -> Double.compare(a.score, b.score)));
        }
        FunctionIterator functions = currentProgram.getFunctionManager().getFunctions(true);
        while (functions.hasNext() && !monitor.isCancelled()) {
            Function function = functions.next();
            List<String> candidate = mnemonics(function);
            Map<String, Integer> other = trigrams(candidate);
            for (int i = 0; i < targets.size(); ++i) {
                List<String> target = targets.get(i);
                if (candidate.size() < target.size() / 3 ||
                    candidate.size() > target.size() * 3) continue;
                int overlap = 0;
                for (Map.Entry<String, Integer> item : references.get(i).entrySet())
                    overlap += Math.min(item.getValue(), other.getOrDefault(item.getKey(), 0));
                double score = 2.0 * overlap /
                    Math.max(1, (target.size() - 2) + (candidate.size() - 2));
                PriorityQueue<Match> best = bests.get(i);
                if (best.size() < 20 || score > best.peek().score) {
                    best.add(new Match(function.getEntryPoint().toString(), candidate.size(), score));
                    if (best.size() > 20) best.remove();
                }
            }
        }
        for (int i = 0; i < bests.size(); ++i) {
            List<Match> ranked = new ArrayList<>(bests.get(i));
            ranked.sort((a, b) -> Double.compare(b.score, a.score));
            for (Match match : ranked)
                println(String.format("TARGET %d MATCH %s %d %.4f", i,
                                      match.address, match.length, match.score));
        }
    }

    private static class Match {
        final String address;
        final int length;
        final double score;
        Match(String address, int length, double score) {
            this.address = address;
            this.length = length;
            this.score = score;
        }
    }
}

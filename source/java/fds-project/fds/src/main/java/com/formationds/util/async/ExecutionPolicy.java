package com.formationds.util.async;

import java.util.Collection;
import java.util.Collections;
import java.util.Iterator;
import java.util.concurrent.CompletableFuture;
import java.util.function.Function;
import java.util.function.Supplier;

public class ExecutionPolicy {
    public static <TTargetType, TResult> CompletableFuture<TResult> failover(Collection<TTargetType> collection, Function<TTargetType, CompletableFuture<TResult>> operation) {
        Iterator<TTargetType> targets = collection.iterator();
        if(!targets.hasNext())
            throw new IllegalArgumentException("target collection is empty");

        CompletableFuture<TResult> result = new CompletableFuture<>();
        failoverInternal(targets, result, operation);
        return result;
    }

    // TODO: generalize this beyond primary/secondary?
    // TODO: the exception returned will be the first failure - do we want to aggregate failure?
    public static <TTargetType> CompletableFuture<Void> parallelPrimarySecondary(Collection<TTargetType> primaries, Collection<TTargetType> secondaries, Function<TTargetType, CompletableFuture<Void>> operation) {
        CompletableFuture<Void> result = CompletableFuture.completedFuture(null);

        for(TTargetType primary :  primaries) {
            CompletableFuture<Void> execution = operation.apply(primary);
            result = result.thenCompose(c -> execution);
        }

        for(TTargetType secondary : secondaries) {
            operation.apply(secondary);
        }

        return result;
    }

    public static <TResult> CompletableFuture<TResult> fixedRetry(int count, Supplier<CompletableFuture<TResult>> operation) {
        if(count < 1)
            throw new IllegalArgumentException("count must be at least 1");

        return failover(Collections.nCopies(count, null), x -> operation.get());
    }

    // continuously retry until shouldRetry returns false
    // shouldRetry does not need to return immediately, which can be used to implement a retry delay (among other things)
    public static <TResult> CompletableFuture<TResult> boundedRetry(Supplier<CompletableFuture<Boolean>> shouldRetry, Supplier<CompletableFuture<TResult>> operation) {
        CompletableFuture<TResult> result = new CompletableFuture<>();
        boundedRetryInternal(shouldRetry, result, operation);
        return result;
    }

    private static <TTargetType, TResult> void boundedRetryInternal(Supplier<CompletableFuture<Boolean>> shouldRetry, CompletableFuture<TResult> result, Supplier<CompletableFuture<TResult>> operation) {
        operation.get()
                .whenComplete((r, ex) -> {
                    if (ex != null) {
                        shouldRetry.get().thenAccept(sr -> {
                            if (sr)
                                boundedRetryInternal(shouldRetry, result, operation);
                            else
                                result.completeExceptionally(ex);

                        }).exceptionally(e -> {
                            result.completeExceptionally(e);
                            return null;
                        });
                    } else {
                        result.complete(r);
                    }
                });
    }

    private static <TTargetType, TResult> void failoverInternal(Iterator<TTargetType> targets, CompletableFuture<TResult> result, Function<TTargetType, CompletableFuture<TResult>> operation) {
        operation.apply(targets.next())
                .whenComplete((r, ex) -> {
                    if (ex == null)
                        result.complete(r);
                    else if (targets.hasNext())
                        failoverInternal(targets, result, operation);
                    else
                        result.completeExceptionally(ex);
                });
    }
}

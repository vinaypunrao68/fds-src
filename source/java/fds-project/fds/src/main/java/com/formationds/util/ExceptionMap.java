package com.formationds.util;

import java.util.ArrayList;
import java.util.concurrent.CompletableFuture;
import java.util.function.Function;

public class ExceptionMap implements Function<Throwable, Throwable> {
    private final ArrayList<Function<Throwable, Throwable>> mappings;

    public ExceptionMap() {
        mappings = new ArrayList<>();
    }

    public ExceptionMap map(Function<Throwable, Throwable> mapping) {
        mappings.add(mapping);
        return this;
    }

    public <T extends Throwable> ExceptionMap map(Class<T> klass, Function<T, Throwable> mapping) {
        mappings.add(f -> {
            if(!klass.isInstance(f))
               return null;
            return mapping.apply(klass.cast(f));
        });
        return this;
    }

    public ExceptionMap mapWhen(Function<Throwable, Boolean> when, Function<Throwable, Throwable> mapping) {
        mappings.add(ex -> when.apply(ex) ? mapping.apply(ex) : null);
        return this;
    }

    public <T extends Throwable> ExceptionMap mapWhen(Class<T> klass, Function<T, Boolean> when, Function<Throwable, Throwable> mapping) {
        map(klass, ex -> when.apply(ex) ? mapping.apply(ex) : null);
        return this;
    }

    public Throwable apply(Throwable in) {
        Throwable out = in;
        for(Function<Throwable, Throwable> mapping : mappings) {
            Throwable ex2 = mapping.apply(out);
            if(ex2 != null) {
                out = ex2;
            }
        }
        return out;
    }

    public <T> CompletableFuture<T> applyOnFail(CompletableFuture<T> future) {
        CompletableFuture<T> result = new CompletableFuture<>();
        future.whenComplete((r, ex) -> {
            if(ex == null) {
                result.complete(r);
                return;
            }

            result.completeExceptionally(apply(ex));
        });
        return result;
    }
}

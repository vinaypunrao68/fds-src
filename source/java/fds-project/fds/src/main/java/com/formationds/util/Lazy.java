package com.formationds.util;

import java.util.function.Supplier;

public class Lazy<T> {
    private final Supplier<T> supplier;
    private volatile T instance;

    public Lazy(Supplier<T> supplier) {
        this.supplier = supplier;
    }

    public T getInstance() {
        if(instance == null) {
            synchronized (supplier) {
                if(instance == null) {
                    instance = supplier.get();
                }
            }
        }
        return instance;
    }
}

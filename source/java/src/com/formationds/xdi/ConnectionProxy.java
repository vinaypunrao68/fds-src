package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.util.function.Supplier;

public class ConnectionProxy<T> {
    private Class<T> klass;
    private Supplier<T> supplier;

    public ConnectionProxy(Class<T> klass, Supplier<T> supplier) {
        this.klass = klass;
        this.supplier = supplier;
    }

    public T makeProxy() {
        return (T) Proxy.newProxyInstance(this.getClass().getClassLoader(), new Class[] {klass}, new InvocationHandler() {
            @Override
            public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
                T tee = supplier.get();
                try {
                    return method.invoke(tee, args);
                } catch (InvocationTargetException e) {
                    throw e.getCause();
                }
            }
        });
    }
}

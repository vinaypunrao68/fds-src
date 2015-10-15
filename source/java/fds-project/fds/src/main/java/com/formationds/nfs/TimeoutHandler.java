package com.formationds.nfs;

import com.formationds.xdi.TimeoutException;
import org.apache.log4j.Logger;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.time.Duration;

public class TimeoutHandler {
    public static final Logger LOG = Logger.getLogger(TimeoutHandler.class);

    public static IoOps buildProxy(IoOps ops, int retryCount, Duration retryInterval) {
        return (IoOps) Proxy.newProxyInstance(TimeoutHandler.class.getClassLoader(), new Class[]{IoOps.class}, new InvocationHandler() {
            @Override
            public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
                for (int i = 0; i < retryCount; i++) {
                    try {
                        return method.invoke(ops, args);
                    } catch (Exception e) {
                        if (e.getCause() instanceof TimeoutException) {
                            Thread.sleep(retryInterval.toMillis());
                            continue;
                        } else {
                            throw e;
                        }
                    }
                }
                TimeoutException e = new TimeoutException();
                LOG.error("Invocation of " + ops.getClass().getSimpleName() + "." + method.getName() + " timed out after " + retryCount + " attempts, aborting", e);
                throw e;
            }
        });
    }
}

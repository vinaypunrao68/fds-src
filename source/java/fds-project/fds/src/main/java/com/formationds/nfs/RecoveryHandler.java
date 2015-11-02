package com.formationds.nfs;

import com.formationds.xdi.RecoverableException;
import org.apache.log4j.Logger;

import java.io.IOException;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.time.Duration;

public class RecoveryHandler {
    public static final Logger LOG = Logger.getLogger(RecoveryHandler.class);

    public static IoOps buildProxy(IoOps ops, int retryCount, Duration retryInterval) {
        return (IoOps) Proxy.newProxyInstance(RecoveryHandler.class.getClassLoader(), new Class[]{IoOps.class}, new InvocationHandler() {
            @Override
            public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
                for (int i = 0; i < retryCount; i++) {
                    try {
                        return method.invoke(ops, args);
                    } catch (Exception e) {
                        if (e.getCause() instanceof RecoverableException) {
                            Thread.sleep(retryInterval.toMillis());
                            continue;
                        } else {
                            throw e;
                        }
                    }
                }
                String message = "Invocation of " + ops.getClass().getSimpleName() + "." + method.getName() + " failed after " + retryCount + " attempts, aborting";
                IOException e = new IOException(message);
                LOG.error(message, e);
                throw e;
            }
        });
    }
}

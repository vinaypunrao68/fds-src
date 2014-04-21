package com.formationds.spike;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.FDSP_MsgHdrType;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;
import java.util.function.Supplier;

public class FdspProxy<Req, Resp> {
    private Class<Req> reqClass;
    private Supplier<Req> supplier;
    private ConcurrentHashMap<String, FdspResponseHandler> handlersBySession;
    private Resp responseProxy;

    public FdspProxy(Class<Req> reqClass, Class<Resp> respClass) {
        this.reqClass = reqClass;
        handlersBySession = new ConcurrentHashMap<>();
        responseProxy = (Resp) Proxy.newProxyInstance(getClass().getClassLoader(), new Class[] {respClass}, new InvocationHandler() {
            @Override
            public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
                System.out.println("Someone is trying to invoke " + method.getName());
                FDSP_MsgHdrType msg = (FDSP_MsgHdrType) args[0];
                String uuid = msg.getSession_uuid();
                handlersBySession.computeIfPresent(uuid, (sessionId, handler) -> {
                    handler.handle(msg, args[1]);
                    handlersBySession.remove(uuid);
                    return handler;
                });
                return null;
            }
        });
    }

    public Resp responseProxy() {
        return responseProxy;
    }

    public <Arg> Req requestProxy(Req realClient, FdspResponseHandler<Arg> responseHandler) {
        return (Req) Proxy.newProxyInstance(getClass().getClassLoader(), new Class[]{reqClass}, new InvocationHandler() {
            @Override
            public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
                Class[] parameterTypes = Arrays.stream(args).map(a -> a.getClass()).toArray(i -> new Class[i]);
                FDSP_MsgHdrType msg = (FDSP_MsgHdrType) args[0];
                msg.setSession_uuid(UUID.randomUUID().toString());
                handlersBySession.put(msg.getSession_uuid(), responseHandler);
                Method realMethod = realClient.getClass().getMethod(method.getName(), parameterTypes);
                realMethod.invoke(realClient, args);
                return null;
            }
        });
    }
}


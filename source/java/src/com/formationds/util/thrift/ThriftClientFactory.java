/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */
package com.formationds.util.thrift;

import org.apache.commons.pool2.KeyedObjectPool;
import org.apache.commons.pool2.impl.GenericKeyedObjectPool;
import org.apache.commons.pool2.impl.GenericKeyedObjectPoolConfig;
import org.apache.log4j.Logger;
import org.apache.thrift.protocol.TProtocolException;
import org.apache.thrift.transport.TTransportException;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.ParameterizedType;
import java.lang.reflect.Proxy;

/**
 *
 * @param <IF>
 */
abstract public class ThriftClientFactory<IF> {
    protected static final Logger LOG = Logger.getLogger(ThriftClientFactory.class);

    /**
     * @param klass
     * @param pool
     * @param host
     * @param port
     * @param <T>
     *
     * @return a remote proxy to the interface specified by klass
     */
    protected static <T> T buildRemoteProxy(Class<T> klass,
                                            KeyedObjectPool<ConnectionSpecification, ThriftClientConnection<T>> pool,
                                            String host,
                                            int port) {
        @SuppressWarnings("unchecked")
        T result = (T) Proxy.newProxyInstance(klass.getClassLoader(), new Class[] {klass},
                                              (proxy, method, args) -> {
                                                  ConnectionSpecification spec =
                                                      new ConnectionSpecification(host, port);
                                                  ThriftClientConnection<T> cnx = pool.borrowObject(spec);
                                                  T client = cnx.getClient();
                                                  try {
                                                      return method.invoke(client, args);
                                                  } catch (InvocationTargetException e) {
                                                      if (e.getCause() instanceof TTransportException ||
                                                          e.getCause() instanceof TProtocolException) {
                                                          pool.invalidateObject(spec, cnx);
                                                          cnx = null;
                                                      }
                                                      throw e.getCause();
                                                  } finally {
                                                      if (cnx != null)
                                                          pool.returnObject(spec, cnx);
                                                  }
                                              });
        return result;
    }

    /**
     * @return the thrift service interface class.
     */
    private Class<IF> getThriftServiceIFaceClass() {
        return (Class<IF>) ((ParameterizedType)getClass().getGenericSuperclass())
                                          .getActualTypeArguments()[0];
    }

    private final GenericKeyedObjectPoolConfig config = new GenericKeyedObjectPoolConfig();
    private final GenericKeyedObjectPool<ConnectionSpecification, ThriftClientConnection<IF>> clientPool;

    /**
     * @param maxPoolSize
     * @param minIdle
     * @param softMinEvictionIdleTimeMillis
     */
    protected ThriftClientFactory(int maxPoolSize, int minIdle, long softMinEvictionIdleTimeMillis) {
        config.setMaxTotal(maxPoolSize);
        config.setMinIdlePerKey(minIdle);
        config.setSoftMinEvictableIdleTimeMillis(softMinEvictionIdleTimeMillis);

        clientPool = createClientPool();
    }

    /**
     * @param host
     * @param port
     *
     * @return a proxy to the thrift service interface
     */
    protected IF buildRemoteProxy(String host,
                                  int port) {
        return (IF)buildRemoteProxy(getThriftServiceIFaceClass(), getClientPool(), host, port);
    }

    /**
     * @return the client connection pool
     */
    abstract protected GenericKeyedObjectPool<ConnectionSpecification,
                                                 ThriftClientConnection<IF>> createClientPool();
    /**
     * @return the pool configuration
     */
    protected GenericKeyedObjectPoolConfig getConfig() { return config; }

    /**
     *
     * @return the client connection pool
     */
    protected GenericKeyedObjectPool<ConnectionSpecification, ThriftClientConnection<IF>> getClientPool() {
        return clientPool;
    }

    /**
     * Make a connection to the specified host and port and return the thrift service interface
     * @param host
     * @param port
     *
     * @return a proxy to the thrift service.
     */
    public IF connect(String host, int port) {
        return buildRemoteProxy(host, port);
    }
}

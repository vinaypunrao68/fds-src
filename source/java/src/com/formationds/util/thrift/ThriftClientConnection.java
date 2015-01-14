/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */
package com.formationds.util.thrift;

import org.apache.thrift.transport.TTransport;

/**
 * @param <T>
 */
public class ThriftClientConnection<T> {
    private TTransport transport;
    private T          client;

    public ThriftClientConnection(TTransport transport, T client) {
        this.transport = transport;
        this.client = client;
    }

    public T getClient() {
        return client;
    }

    public boolean valid() {
        return transport.isOpen();
    }

    public void close() {
        try {
            transport.close();
        } catch(Exception e) {
            // do nothing - we made a best effort to close
        }
    }
}

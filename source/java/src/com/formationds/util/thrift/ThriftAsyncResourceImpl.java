/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */
package com.formationds.util.thrift;

import com.formationds.util.async.AsyncResourcePool;
import org.apache.thrift.transport.TNonblockingTransport;

import java.util.function.Function;

public class ThriftAsyncResourceImpl<T> implements AsyncResourcePool.Impl<ThriftClientConnection<T>> {
    private Function<TNonblockingTransport, T> constructor;
    private ConnectionSpecification            specification;

    public ThriftAsyncResourceImpl(ConnectionSpecification cs, Function<TNonblockingTransport, T> constructor) {
        this.constructor = constructor;
        this.specification = cs;
    }

    @Override
    public ThriftClientConnection<T> construct() throws Exception {
        return ThriftClientConnectionFactory.makeAsyncConnection(specification, constructor);
    }

    @Override
    public void destroy(ThriftClientConnection<T> elt) {
        elt.close();
    }

    @Override
    public boolean isValid(ThriftClientConnection<T> elt) {
        return elt.valid();
    }
}

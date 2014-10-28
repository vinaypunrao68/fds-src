import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.transport.*;

import java.io.IOException;
import java.util.function.Function;

class ConnectionSpecification {
    public String host;
    public int port;

    public ConnectionSpecification(String host, int port) {
        this.host = host;
        this.port = port;
    }

    public <T> XdiClientConnection<T> makeAsyncConnection(Function<TNonblockingTransport, T> constructor)  throws IOException {
        TNonblockingTransport transport = new TNonblockingSocket(host, port);
        return new XdiClientConnection<T>(transport, constructor.apply(transport));
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        ConnectionSpecification that = (ConnectionSpecification) o;

        if (port != that.port) return false;
        if (!host.equals(that.host)) return false;

        return true;
    }

    @Override
    public int hashCode() {
        int result = host.hashCode();
        result = 31 * result + port;
        return result;
    }
}

class XdiClientConnection<T> {
    private TTransport transport;
    private T client;

    public XdiClientConnection(TTransport transport, T client) {
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

class XdiAsyncResourceImpl<T> implements AsyncResourcePool.Impl<XdiClientConnection<T>> {
    private Function<TNonblockingTransport, T> constructor;
    private ConnectionSpecification specification;

    public XdiAsyncResourceImpl(ConnectionSpecification cs, Function<TNonblockingTransport, T> constructor) {
        this.constructor = constructor;
        this.specification = cs;
    }

    @Override
    public XdiClientConnection<T> construct() throws Exception {
        return specification.makeAsyncConnection(constructor);
    }

    @Override
    public void destroy(XdiClientConnection<T> elt) {
        elt.close();
    }

    @Override
    public boolean isValid(XdiClientConnection<T> elt) {
        return elt.valid();
    }
}
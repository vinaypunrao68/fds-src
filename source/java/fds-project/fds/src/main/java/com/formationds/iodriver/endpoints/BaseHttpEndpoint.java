package com.formationds.iodriver.endpoints;

import java.nio.charset.Charset;

import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.operations.BaseHttpOperation;
import com.formationds.iodriver.reporters.AbstractWorkloadEventListener;

/**
 * An endpoint that targets an HTTP server.
 *
 * @param <ConnectionT> The type of connection supplied by this endpoint.
 */
public interface BaseHttpEndpoint<ConnectionT> extends Endpoint
{
    /**
     * Read from a connection.
     * 
     * @param connection The connection to read the response of.
     *
     * @return The content returned by the server.
     *
     * @throws HttpException when an error occurs sending the request or receiving the response.
     */
    String doRead(ConnectionT connection) throws HttpException;

    /**
     * Write to a connection without reading from it. Response code (and error response if not
     * successful) will still be read.
     *
     * @param connection PUT/POST/etc. here.
     * @param content The content to write.
     * @param charset {@code content} is in this charset.
     *
     * @throws HttpException when an error occurs sending the request or receiving the respsonse.
     */
    void doWrite(ConnectionT connection, String content, Charset charset) throws HttpException;

    String doWriteThenRead(ConnectionT connection,
                           String content,
                           Charset charset) throws HttpException;
    
    Class<ConnectionT> getConnectionType();

    /**
     * Execute an an operation that requires an HTTP connection.
     *
     * @param operation The operation to execute.
     * @param listener Report progress here.
     *
     * @throws ExecutionException when an error occurs.
     */
    void visit(BaseHttpOperation<ConnectionT> operation,
               AbstractWorkloadEventListener listener) throws ExecutionException;
}

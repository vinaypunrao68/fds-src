package com.formationds.iodriver.endpoints;

import java.nio.charset.Charset;

import com.formationds.iodriver.operations.BaseHttpOperation;
import com.formationds.iodriver.operations.ExecutionException;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

public interface BaseHttpEndpoint<ConnectionT> extends Endpoint
{
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
    
    void visit(BaseHttpOperation<ConnectionT> operation,
               AbstractWorkflowEventListener listener) throws ExecutionException;
}

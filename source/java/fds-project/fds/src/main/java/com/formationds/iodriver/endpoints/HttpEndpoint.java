package com.formationds.iodriver.endpoints;

import java.net.HttpURLConnection;

import com.formationds.iodriver.operations.HttpOperation;

public interface HttpEndpoint<ThisT extends HttpEndpoint<ThisT, OperationT>,
                              OperationT extends HttpOperation<OperationT, ? super ThisT>>
extends BaseHttpEndpoint<ThisT, OperationT, HttpURLConnection>
{
    
}

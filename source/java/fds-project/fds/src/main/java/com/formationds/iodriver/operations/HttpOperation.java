package com.formationds.iodriver.operations;

import java.net.HttpURLConnection;

import com.formationds.iodriver.endpoints.AbstractHttpEndpoint;

public interface HttpOperation<ThisT extends HttpOperation<ThisT, EndpointT>,
                               EndpointT extends AbstractHttpEndpoint<EndpointT, ? super ThisT>>
extends BaseHttpOperation<ThisT, EndpointT, HttpURLConnection>
{ }

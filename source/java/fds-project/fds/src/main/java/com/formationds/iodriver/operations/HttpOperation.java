package com.formationds.iodriver.operations;

import java.net.HttpURLConnection;

import com.formationds.iodriver.endpoints.BaseHttpEndpoint;

public interface HttpOperation<ThisT extends HttpOperation<ThisT, EndpointT>,
                               EndpointT extends BaseHttpEndpoint<EndpointT,
                                                                  ? super ThisT,
                                                                  ? extends HttpURLConnection>>
extends BaseHttpOperation<ThisT, EndpointT, HttpURLConnection>
{ }

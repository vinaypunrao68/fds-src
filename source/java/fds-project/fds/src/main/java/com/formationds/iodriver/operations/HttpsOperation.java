package com.formationds.iodriver.operations;

import javax.net.ssl.HttpsURLConnection;

import com.formationds.iodriver.endpoints.AbstractHttpsEndpoint;

public interface HttpsOperation<ThisT extends HttpsOperation<ThisT, EndpointT>,
                                EndpointT extends AbstractHttpsEndpoint<EndpointT, ? super ThisT>>
extends BaseHttpOperation<ThisT, EndpointT, HttpsURLConnection>
{ }

package com.formationds.iodriver.operations;

import javax.net.ssl.HttpsURLConnection;

/**
 * Much the same as {@link AbstractHttpOperation}, but only HTTPS connections are allowed.
 * 
 * @param <ThisT> The implementing class.
 * @param <EndpointT> The type of endpoint this operation can run on.
 */
// @eclipseFormat:off
public abstract class AbstractHttpsOperation extends AbstractBaseHttpOperation<HttpsURLConnection>
                                             implements HttpsOperation
// @eclipseFormat:on
{
    protected AbstractHttpsOperation()
    {
        super(HttpsURLConnection.class);
    }
}

package com.formationds.iodriver.operations;

import java.net.HttpURLConnection;

/**
 * Basic operation that runs on an HTTP endpoint.
 */
// @eclipseFormat:off
public abstract class AbstractHttpOperation extends AbstractBaseHttpOperation<HttpURLConnection>
                                            implements HttpOperation
// @eclipseFormat:on
{
    protected AbstractHttpOperation()
    {
        super(HttpURLConnection.class);
    }
}

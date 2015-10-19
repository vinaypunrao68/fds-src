package com.formationds.iodriver.endpoints;

import javax.net.ssl.HttpsURLConnection;

/**
 * An endpoint that targets an HTTPS server.
 */
public interface HttpsEndpoint extends BaseHttpEndpoint<HttpsURLConnection>
{ }

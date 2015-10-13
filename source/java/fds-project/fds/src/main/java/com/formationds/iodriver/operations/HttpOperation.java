package com.formationds.iodriver.operations;

import java.net.HttpURLConnection;

import com.formationds.iodriver.WorkloadContext;
import com.formationds.iodriver.endpoints.HttpEndpoint;

public interface HttpOperation extends BaseHttpOperation<HttpURLConnection>
{
    void accept(HttpEndpoint endpoint,
                HttpURLConnection connection,
                WorkloadContext context);
}

package com.formationds.iodriver.operations;

import java.net.HttpURLConnection;

import com.formationds.iodriver.endpoints.HttpEndpoint;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

public interface HttpOperation extends BaseHttpOperation<HttpURLConnection>
{
    void accept(HttpEndpoint endpoint,
                HttpURLConnection connection,
                AbstractWorkflowEventListener listener);
}

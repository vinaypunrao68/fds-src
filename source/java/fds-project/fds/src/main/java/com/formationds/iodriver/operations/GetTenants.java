package com.formationds.iodriver.operations;

import javax.net.ssl.HttpsURLConnection;

import com.formationds.iodriver.endpoints.OrchestrationManagerEndpoint;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

public class GetTenants extends OrchestrationManagerOperation {

	@Override
	public void exec(OrchestrationManagerEndpoint endpoint,
			HttpsURLConnection connection,
			AbstractWorkflowEventListener reporter) throws ExecutionException {
		// TODO Auto-generated method stub

	}

}

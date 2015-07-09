package com.formationds.iodriver.operations;

import java.net.URI;
import java.nio.charset.StandardCharsets;

import javax.net.ssl.HttpsURLConnection;

import com.formationds.client.v08.model.Volume;
import com.formationds.commons.Fds;
import com.formationds.commons.NullArgumentException;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.iodriver.endpoints.HttpException;
import com.formationds.iodriver.endpoints.OrchestrationManagerEndpoint;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

public class CreateVolume extends OrchestrationManagerOperation
{
	public CreateVolume(String name)
	{
		if (name == null) throw new NullArgumentException("name");
		
		_name = name;
	}
	
	@Override
	public void exec(OrchestrationManagerEndpoint endpoint,
			HttpsURLConnection connection,
			AbstractWorkflowEventListener reporter) throws ExecutionException
	{
		if (endpoint == null) throw new NullArgumentException("endpoint");
		if (connection == null) throw new NullArgumentException("connection");
		if (reporter == null) throw new NullArgumentException("reporter");

		Volume newVolume = new Volume.Builder(_name).create();
		
		try
		{
			endpoint.doWrite(connection,
			                 ObjectModelHelper.toJSON(newVolume),
			                 StandardCharsets.UTF_8);
		}
		catch (HttpException e)
		{
			throw new ExecutionException(e);
		}
	}
	
	@Override
	public URI getRelativeUri()
	{
		URI base = Fds.Api.V08.getBase();
		URI createVolume = Fds.Api.V08.getVolumes();
		
		return base.relativize(createVolume);
	}
	
	@Override
	public String getRequestMethod()
	{
	    return "POST";
	}

	private final String _name;
}

package com.formationds.iodriver.operations;

import java.lang.reflect.Type;
import java.net.URI;
import java.util.ArrayList;
import java.util.Collection;
import java.util.function.Consumer;

import javax.net.ssl.HttpsURLConnection;

import com.formationds.client.v08.model.Tenant;
import com.formationds.commons.Fds;
import com.formationds.commons.NullArgumentException;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.endpoints.HttpException;
import com.formationds.iodriver.endpoints.OmV8Endpoint;
import com.formationds.iodriver.reporters.WorkloadEventListener;
import com.google.gson.reflect.TypeToken;

public class GetTenants extends AbstractOmV8Operation
{
	public GetTenants(Consumer<Collection<Tenant>> reader)
	{
		if (reader == null) throw new NullArgumentException("reader");
		
		_setter = reader;
	}
	
	@Override
	public void accept(OmV8Endpoint endpoint,
	                   HttpsURLConnection connection,
			           WorkloadEventListener reporter) throws ExecutionException
	{
		if (endpoint == null) throw new NullArgumentException("endpoint");
		if (connection == null) throw new NullArgumentException("connection");
		if (reporter == null) throw new NullArgumentException("reporter");
		
		try
		{
			_setter.accept(ObjectModelHelper.toObject(endpoint.doRead(connection),
					                                  _TENANT_LIST_TYPE));
		}
		catch (HttpException e)
		{
			throw new ExecutionException(e);
		}
	}
    
    @Override
    public URI getRelativeUri()
    {
        URI apiBase = Fds.Api.V08.getBase();
        URI tenants = Fds.Api.V08.getTenants();
        
        return apiBase.relativize(tenants);
    }

    @Override
    public String getRequestMethod()
    {
        return "GET";
    }

    static
    {
    	_TENANT_LIST_TYPE = new TypeToken<ArrayList<Tenant>>() {}.getType();
    }
    
	private final Consumer<Collection<Tenant>> _setter;
	
	private static final Type _TENANT_LIST_TYPE;
}

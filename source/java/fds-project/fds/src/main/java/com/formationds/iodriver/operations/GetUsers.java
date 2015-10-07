package com.formationds.iodriver.operations;

import java.lang.reflect.Type;
import java.net.URI;
import java.util.ArrayList;
import java.util.Collection;
import java.util.function.Consumer;

import javax.net.ssl.HttpsURLConnection;

import com.formationds.client.v08.model.User;
import com.formationds.commons.Fds;
import com.formationds.commons.NullArgumentException;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.WorkloadContext;
import com.formationds.iodriver.endpoints.HttpException;
import com.formationds.iodriver.endpoints.OmV8Endpoint;
import com.google.gson.reflect.TypeToken;

public final class GetUsers extends AbstractOmV8Operation
{
    public GetUsers(Consumer<Collection<User>> setter)
    {
        if (setter == null) throw new NullArgumentException("setter");
        
        _setter = setter;
    }
    
    @Override
    public void accept(OmV8Endpoint endpoint,
                       HttpsURLConnection connection,
                       WorkloadContext context) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (connection == null) throw new NullArgumentException("connection");
        if (context == null) throw new NullArgumentException("context");
        
        String users;
        try
        {
            users = endpoint.doRead(connection);
        }
        catch (HttpException e)
        {
            throw new ExecutionException("Error listing users.", e);
        }

        _setter.accept(ObjectModelHelper.toObject(users, _USER_LIST_TYPE));
    }
    
    @Override
    public URI getRelativeUri()
    {
        URI base = Fds.Api.V08.getBase();
        URI getUsers = Fds.Api.V08.getUsers();
        
        return base.relativize(getUsers);
    }
    
    @Override
    public String getRequestMethod()
    {
        return "GET";
    }
    
    static
    {
        _USER_LIST_TYPE = new TypeToken<ArrayList<User>>() {}.getType();
    }
    
    private final Consumer<Collection<User>> _setter;
    
    private static final Type _USER_LIST_TYPE;
}

package com.formationds.iodriver.operations;

import java.net.URI;
import java.util.function.Consumer;

import javax.net.ssl.HttpsURLConnection;

import com.formationds.client.v08.model.Volume;
import com.formationds.commons.Fds;
import com.formationds.commons.NullArgumentException;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.commons.util.Uris;
import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.WorkloadContext;
import com.formationds.iodriver.endpoints.HttpException;
import com.formationds.iodriver.endpoints.OmV8Endpoint;

public final class GetVolume extends AbstractOmV8Operation
{
    public GetVolume(long id, Consumer<? super Volume> setter)
    {
        if (setter == null) throw new NullArgumentException("setter");
        
        _id = id;
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
        
        try
        {
            _setter.accept(ObjectModelHelper.toObject(endpoint.doRead(connection), Volume.class));
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
        URI volumes = Fds.Api.V08.getVolumes();
        URI volumeId = Uris.tryGetRelativeUri(Long.toString(_id));
        URI getVolume = Uris.resolve(volumes, volumeId);
        
        return apiBase.relativize(getVolume);
    }

    @Override
    public String getRequestMethod()
    {
        return "GET";
    }

    private final long _id;
    
    private final Consumer<? super Volume> _setter;
}

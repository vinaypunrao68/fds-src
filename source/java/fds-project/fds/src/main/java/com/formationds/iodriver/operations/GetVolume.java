package com.formationds.iodriver.operations;

import java.net.URI;
import java.util.function.Consumer;

import javax.net.ssl.HttpsURLConnection;

import com.formationds.client.v08.model.Volume;
import com.formationds.commons.Fds;
import com.formationds.commons.NullArgumentException;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.commons.util.Uris;
import com.formationds.iodriver.endpoints.HttpException;
import com.formationds.iodriver.endpoints.OrchestrationManagerEndpoint;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

public final class GetVolume extends OrchestrationManagerOperation
{
    public GetVolume(long id, Consumer<? super Volume> setter)
    {
        if (setter == null) throw new NullArgumentException("setter");
        
        _id = id;
        _setter = setter;
    }
    
    @Override
    public void exec(OrchestrationManagerEndpoint endpoint,
                     HttpsURLConnection connection,
                     AbstractWorkflowEventListener reporter) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (connection == null) throw new NullArgumentException("connection");
        if (reporter == null) throw new NullArgumentException("reporter");
        
        try
        {
            _setter.accept(ObjectModelHelper.toObject(endpoint.doGet(connection), Volume.class));
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
    
    private final long _id;
    
    private final Consumer<? super Volume> _setter;
}

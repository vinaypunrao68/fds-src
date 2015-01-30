package com.formationds.iodriver.operations;

import java.net.URI;
import java.nio.charset.StandardCharsets;

import javax.net.ssl.HttpsURLConnection;

import org.json.JSONObject;

import com.formationds.apis.MediaPolicy;
import com.formationds.commons.FdsConstants;
import com.formationds.commons.NullArgumentException;
import com.formationds.commons.util.Uris;
import com.formationds.iodriver.endpoints.HttpException;
import com.formationds.iodriver.endpoints.OrchestrationManagerEndpoint;

public final class SetVolumeQos extends OrchestrationManagerOperation
{
    public static class Input
    {
        public final int assured_rate;
        public final long commit_log_retention;
        public final long id;
        public final MediaPolicy mediaPolicy;
        public final int priority;
        public final int throttle_rate;

        public Input(long id,
                     int assured_rate,
                     int throttle_rate,
                     int priority,
                     long commit_log_retention,
                     MediaPolicy mediaPolicy)
        {
            this.id = id;
            this.assured_rate = assured_rate;
            this.throttle_rate = throttle_rate;
            this.priority = priority;
            this.commit_log_retention = commit_log_retention;
            this.mediaPolicy = mediaPolicy;
        }
    }

    public SetVolumeQos(Input input)
    {
        if (input == null) throw new NullArgumentException("input");

        _input = input;
    }

    // @eclipseFormat:off
    @Override
    public void exec(OrchestrationManagerEndpoint endpoint, HttpsURLConnection connection)
            throws ExecutionException
    // @eclipseFormat:on
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (connection == null) throw new NullArgumentException("connection");

        connection.setDoInput(false);
        connection.setDoOutput(true);

        JSONObject request = new JSONObject();
        request.put("sla", _input.assured_rate);
        request.put("priority", _input.priority);
        request.put("limit", _input.throttle_rate);
        request.put("commit_log_retention", _input.commit_log_retention);
        request.put("mediaPolicy", _input.mediaPolicy.toString());

        try
        {
            endpoint.doPut(connection, request.toString(), StandardCharsets.UTF_8);
        }
        catch (HttpException e)
        {
            throw new ExecutionException(e);
        }
    }

    @Override
    public URI getRelativeUri()
    {
        return Uris.resolve(FdsConstants.Api.getVolumes(),
                            Uris.tryGetRelativeUri(Long.toString(_input.id)))
                   .relativize(FdsConstants.Api.getBase());
    }

    private final Input _input;
}

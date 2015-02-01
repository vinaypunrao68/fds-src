package com.formationds.iodriver.operations;

import java.net.URI;
import java.util.function.Consumer;

import javax.net.ssl.HttpsURLConnection;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import com.formationds.apis.MediaPolicy;
import com.formationds.commons.Fds;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.HttpException;
import com.formationds.iodriver.endpoints.OrchestrationManagerEndpoint;
import com.formationds.iodriver.reporters.VerificationReporter;

public final class StatVolume extends OrchestrationManagerOperation
{
    public static class Output
    {
        public final int assured_rate;
        public final long commit_log_retention;
        public final long id;
        public final MediaPolicy mediaPolicy;
        public final int throttle_rate;
        public final int priority;

        public Output(int assured_rate,
                      int throttle_rate,
                      int priority,
                      long commit_log_retention,
                      MediaPolicy mediaPolicy,
                      long id)
        {
            this.assured_rate = assured_rate;
            this.commit_log_retention = commit_log_retention;
            this.mediaPolicy = mediaPolicy;
            this.throttle_rate = throttle_rate;
            this.priority = priority;
            this.id = id;
        }
        // /api/config/volumes/:uuid
        // {
        // "sla": int
        // "priority": int
        // "limit" : int
        // "commit_log_retention": long (seconds)
        // "mediaPolicy": MediaPolicy
        // }
    }

    public StatVolume(String volumeName, Consumer<Output> consumer)
    {
        if (volumeName == null) throw new NullArgumentException("volumeName");
        if (consumer == null) throw new NullArgumentException("consumer");

        _consumer = consumer;
        _volumeName = volumeName;
    }

    @Override
    public void exec(OrchestrationManagerEndpoint endpoint,
                     HttpsURLConnection connection,
                     VerificationReporter reporter) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (connection == null) throw new NullArgumentException("connection");
        if (reporter == null) throw new NullArgumentException("reporter");

        String content;
        try
        {
            content = endpoint.doGet(connection);
        }
        catch (HttpException e)
        {
            throw new ExecutionException(e);
        }

        JSONArray volumes;
        try
        {
            volumes = new JSONArray(content);
        }
        catch (JSONException e)
        {
            throw new ExecutionException("Error parsing response: " + content, e);
        }

        boolean found = false;
        for (int i = 0; i != volumes.length(); ++i)
        {
            // FIXME: Need to deal with tenants.
            JSONObject voldesc = volumes.optJSONObject(i);
            if (voldesc != null && voldesc.getString("name").equals(_volumeName))
            {
                JSONObject policy = voldesc.getJSONObject("policy");

                int assured_rate = voldesc.getInt("sla");
                int throttle_rate = voldesc.getInt("limit");
                int priority = voldesc.getInt("priority");
                long commit_log_retention = voldesc.getLong("commit_log_retention");
                MediaPolicy mediaPolicy = MediaPolicy.valueOf(policy.getString("mediaPolicy"));
                long id = Long.parseLong(voldesc.getString("id"));

                _consumer.accept(new Output(assured_rate,
                                            throttle_rate,
                                            priority,
                                            commit_log_retention,
                                            mediaPolicy,
                                            id));
                found = true;
                break;
            }
        }
        if (!found)
        {
            throw new ExecutionException("Volume " + _volumeName + " not found.");
        }
    }

    @Override
    public URI getRelativeUri()
    {
        return Fds.Api.getBase().relativize(Fds.Api.getVolumes());
    }

    private final Consumer<Output> _consumer;

    private final String _volumeName;
}

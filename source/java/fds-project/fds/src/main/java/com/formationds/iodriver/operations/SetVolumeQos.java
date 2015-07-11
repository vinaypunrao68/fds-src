package com.formationds.iodriver.operations;

import java.net.URI;
import java.nio.charset.StandardCharsets;
import java.util.AbstractMap.SimpleImmutableEntry;
import java.util.stream.Stream;

import javax.net.ssl.HttpsURLConnection;

import org.json.JSONObject;

import com.formationds.client.v08.model.Volume;
import com.formationds.commons.Fds;
import com.formationds.commons.NullArgumentException;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.commons.util.Uris;
import com.formationds.iodriver.endpoints.HttpException;
import com.formationds.iodriver.endpoints.OrchestrationManagerEndpoint;
import com.formationds.iodriver.model.VolumeQosSettings;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

/**
 * Set the QoS parameters on a volume.
 */
public final class SetVolumeQos extends OrchestrationManagerOperation
{
    /**
     * Constructor.
     * 
     * @param input The parameters to set.
     */
    public SetVolumeQos(VolumeQosSettings input)
    {
        if (input == null) throw new NullArgumentException("input");

        _input = input;
    }

    @Override
    public void exec(OrchestrationManagerEndpoint endpoint,
                     HttpsURLConnection connection,
                     AbstractWorkflowEventListener reporter) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (connection == null) throw new NullArgumentException("connection");
        if (reporter == null) throw new NullArgumentException("reporter");

        Volume[] volumeQosSetter = new Volume[1];
        GetVolume getter = new GetVolume(_input.getId(), volume -> volumeQosSetter[0] = volume);
        endpoint.doVisit(getter, reporter);

        JSONObject oyVeh = new JSONObject(ObjectModelHelper.toJSON(volumeQosSetter[0]));
        JSONObject qos = oyVeh.getJSONObject("qosPolicy");
        qos.put("priority", _input.getPriority());
        qos.put("iopsMin", _input.getIopsAssured());
        qos.put("iopsMax", _input.getIopsThrottle());

        try
        {
            endpoint.doPut(connection, oyVeh.toString(), StandardCharsets.UTF_8);
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
        URI volumeId = Uris.tryGetRelativeUri(Long.toString(_input.getId()));
        URI putVolume = Uris.resolve(volumes, volumeId);

        return apiBase.relativize(putVolume);
    }

    @Override
    public Stream<SimpleImmutableEntry<String, String>> toStringMembers()
    {
        return Stream.concat(super.toStringMembers(),
                             Stream.of(memberToString("input", _input)));
    }
    
    /**
     * The parameters to set.
     */
    private final VolumeQosSettings _input;
}

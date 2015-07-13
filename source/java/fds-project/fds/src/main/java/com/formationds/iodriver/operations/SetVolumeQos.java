package com.formationds.iodriver.operations;

import java.net.URI;
import java.nio.charset.StandardCharsets;
import java.util.AbstractMap.SimpleImmutableEntry;
import java.util.function.Supplier;
import java.util.stream.Stream;

import javax.net.ssl.HttpsURLConnection;

import org.json.JSONObject;

import com.formationds.client.v08.model.Volume;
import com.formationds.commons.Fds;
import com.formationds.commons.NullArgumentException;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.commons.util.Uris;
import com.formationds.iodriver.endpoints.HttpException;
import com.formationds.iodriver.endpoints.OmEndpoint;
import com.formationds.iodriver.model.VolumeQosSettings;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

/**
 * Set the QoS parameters on a volume.
 */
public final class SetVolumeQos extends AbstractOmOperation
{
    /**
     * Constructor.
     * 
     * @param input The parameters to set.
     */
    public SetVolumeQos(Supplier<VolumeQosSettings> input)
    {
        if (input == null) throw new NullArgumentException("input");

        _input = input;
    }

    @Override
    public void accept(OmEndpoint endpoint,
                       HttpsURLConnection connection,
                       AbstractWorkflowEventListener reporter) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (connection == null) throw new NullArgumentException("connection");
        if (reporter == null) throw new NullArgumentException("reporter");

        VolumeQosSettings settings = _input.get();
        
        Volume[] volumeQosSetter = new Volume[1];
        GetVolume getter = new GetVolume(settings.getId(), volume -> volumeQosSetter[0] = volume);
        endpoint.visit(getter, reporter);

        JSONObject oyVeh = new JSONObject(ObjectModelHelper.toJSON(volumeQosSetter[0]));
        JSONObject qos = oyVeh.getJSONObject("qosPolicy");
        qos.put("priority", settings.getPriority());
        qos.put("iopsMin", settings.getIopsAssured());
        qos.put("iopsMax", settings.getIopsThrottle());

        try
        {
            endpoint.doWrite(connection, oyVeh.toString(), StandardCharsets.UTF_8);
        }
        catch (HttpException e)
        {
            throw new ExecutionException(e);
        }
    }

    @Override
    public URI getRelativeUri()
    {
        VolumeQosSettings settings = _input.get();
        
        URI apiBase = Fds.Api.V08.getBase();
        URI volumes = Fds.Api.V08.getVolumes();
        URI volumeId = Uris.tryGetRelativeUri(Long.toString(settings.getId()));
        URI putVolume = Uris.resolve(volumes, volumeId);

        return apiBase.relativize(putVolume);
    }

    @Override
    public String getRequestMethod()
    {
        return "PUT";
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
    private final Supplier<VolumeQosSettings> _input;
}

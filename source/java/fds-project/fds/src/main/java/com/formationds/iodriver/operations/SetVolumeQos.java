package com.formationds.iodriver.operations;

import java.net.URI;
import java.nio.charset.StandardCharsets;
import java.time.Instant;
import java.util.AbstractMap.SimpleImmutableEntry;
import java.util.function.Supplier;
import java.util.stream.Stream;

import javax.net.ssl.HttpsURLConnection;

import org.json.JSONObject;

import com.formationds.client.v08.model.QosPolicy;
import com.formationds.client.v08.model.Volume;
import com.formationds.commons.Fds;
import com.formationds.commons.NullArgumentException;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.commons.util.Uris;
import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.WorkloadContext;
import com.formationds.iodriver.endpoints.HttpException;
import com.formationds.iodriver.endpoints.OmV8Endpoint;
import com.formationds.iodriver.events.VolumeModified;
import com.formationds.iodriver.model.VolumeQosSettings;
import com.formationds.iodriver.reporters.BeforeAfter;

/**
 * Set the QoS parameters on a volume.
 */
public final class SetVolumeQos extends AbstractOmV8Operation
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
    public void accept(OmV8Endpoint endpoint,
                       HttpsURLConnection connection,
                       WorkloadContext context) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (connection == null) throw new NullArgumentException("connection");
        if (context == null) throw new NullArgumentException("context");

        VolumeQosSettings settings = _input.get();
        
        Volume[] volumeQosSetter = new Volume[1];
        GetVolume getter = new GetVolume(settings.getId(), volume -> volumeQosSetter[0] = volume);
        endpoint.visit(getter, context);

        Volume oldVolume = volumeQosSetter[0];
        
        JSONObject oyVeh = new JSONObject(ObjectModelHelper.toJSON(volumeQosSetter[0]));
        JSONObject qos = oyVeh.getJSONObject("qosPolicy");
        qos.put("priority", settings.getPriority());
        qos.put("iopsMin", settings.getIopsAssured());
        qos.put("iopsMax", settings.getIopsThrottle());

        String modifiedVolumeString;
        try
        {
            modifiedVolumeString = endpoint.doWriteThenRead(connection,
                                                            oyVeh.toString(),
                                                            StandardCharsets.UTF_8);
        }
        catch (HttpException e)
        {
            throw new ExecutionException(e);
        }
        Volume modifiedVolume = ObjectModelHelper.toObject(modifiedVolumeString, Volume.class);

        QosPolicy modifiedQos = modifiedVolume.getQosPolicy();
        if (modifiedQos.getPriority() == settings.getPriority()
            && modifiedQos.getIopsMin() == settings.getIopsAssured()
            && modifiedQos.getIopsMax() == settings.getIopsThrottle())
        {
            context.sendIfRegistered(new VolumeModified(Instant.now(),
                                                        new BeforeAfter<>(oldVolume,
                                                                          modifiedVolume)));
        }
        else
        {
            throw new ExecutionException("QoS was not updated to requested settings: " + settings
                                         + ", got QosPolicy(priority: " + modifiedQos.getPriority()
                                         + ", iopsMin: " + modifiedQos.getIopsMin()
                                         + ", iopsMax: " + modifiedQos.getIopsMax()
                                         + ") instead.");
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

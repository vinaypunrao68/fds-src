package com.formationds.iodriver.operations;

import java.lang.reflect.Type;
import java.net.URI;
import java.time.Instant;
import java.util.AbstractMap.SimpleImmutableEntry;
import java.util.List;
import java.util.function.Consumer;
import java.util.stream.Stream;

import javax.net.ssl.HttpsURLConnection;

import com.formationds.client.v08.model.Volume;
import com.formationds.commons.Fds;
import com.formationds.commons.NullArgumentException;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.WorkloadContext;
import com.formationds.iodriver.endpoints.HttpException;
import com.formationds.iodriver.endpoints.OmV8Endpoint;
import com.formationds.iodriver.events.VolumeStatted;
import com.formationds.iodriver.model.VolumeQosSettings;
import com.google.common.reflect.TypeToken;

/**
 * Get the QoS settings for a volume.
 */
@SuppressWarnings("serial")
public final class StatVolume extends AbstractOmV8Operation
{
    /**
     * Constructor.
     * 
     * @param volumeName The name of the volume to stat.
     * @param consumer Where to send the settings.
     */
    public StatVolume(String volumeName, Consumer<VolumeQosSettings> consumer)
    {
        if (volumeName == null) throw new NullArgumentException("volumeName");
        if (consumer == null) throw new NullArgumentException("consumer");

        _consumer = consumer;
        _volumeName = volumeName;
    }

    @Override
    public void accept(OmV8Endpoint endpoint,
                       HttpsURLConnection connection,
                       WorkloadContext context) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (connection == null) throw new NullArgumentException("connection");
        if (context == null) throw new NullArgumentException("context");

        String content;
        try
        {
            content = endpoint.doRead(connection);
        }
        catch (HttpException e)
        {
            throw new ExecutionException(e);
        }

        List<Volume> volumes = ObjectModelHelper.toObject(content, _VOLUME_LIST_TYPE);

        boolean found = false;
        for (Volume volume : volumes)
        {
            context.sendIfRegistered(new VolumeStatted(Instant.now(), volume));
            
            // FIXME: Need to deal with tenants.
            if (volume.getName().equals(_volumeName))
            {
                _consumer.accept(VolumeQosSettings.fromVolume(volume));
                
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
        return Fds.Api.V08.getBase().relativize(Fds.Api.V08.getVolumes());
    }

    @Override
    public String getRequestMethod()
    {
        return "GET";
    }

    @Override
    public Stream<SimpleImmutableEntry<String, String>> toStringMembers()
    {
        return Stream.concat(super.toStringMembers(),
                             Stream.of(memberToString("consumer", _consumer),
                                       memberToString("volumeName", _volumeName)));
    }
    
    static
    {
        _VOLUME_LIST_TYPE = new TypeToken<List<Volume>>() {}.getType();
    }
    
    /**
     * Where to send the settings.
     */
    private final Consumer<VolumeQosSettings> _consumer;

    /**
     * The name of the volume to stat.
     */
    private final String _volumeName;
    
    private static final Type _VOLUME_LIST_TYPE;
}

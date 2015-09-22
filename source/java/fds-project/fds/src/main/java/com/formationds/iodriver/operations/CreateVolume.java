package com.formationds.iodriver.operations;

import java.net.URI;
import java.nio.charset.StandardCharsets;
import java.util.AbstractMap.SimpleImmutableEntry;
import java.util.stream.Stream;

import javax.net.ssl.HttpsURLConnection;

import com.formationds.client.v08.model.MediaPolicy;
import com.formationds.client.v08.model.QosPolicy;
import com.formationds.client.v08.model.TimeUnit;
import com.formationds.client.v08.model.Volume;
import com.formationds.client.v08.model.VolumeSettingsObject;
import com.formationds.commons.Fds;
import com.formationds.commons.NullArgumentException;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.endpoints.HttpException;
import com.formationds.iodriver.endpoints.OmV8Endpoint;
import com.formationds.iodriver.model.VolumeQosSettings;
import com.formationds.iodriver.reporters.AbstractWorkloadEventListener;
import com.formationds.iodriver.reporters.WorkloadEventListener;

public class CreateVolume extends AbstractOmV8Operation
{
	public CreateVolume(String name)
	{
		if (name == null) throw new NullArgumentException("name");
		
		_name = name;
	}

    @Override
    public void accept(OmV8Endpoint endpoint,
                       HttpsURLConnection connection,
                       AbstractWorkloadEventListener listener) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (connection == null) throw new NullArgumentException("connection");
        if (listener == null) throw new NullArgumentException("listener");

        // Other than name, the values below are present only to prevent NullPointerExceptions
        // when XDI converts the request.
        Volume.Builder newVolumeBuilder = new Volume.Builder(_name);
        newVolumeBuilder.settings(new VolumeSettingsObject());
        newVolumeBuilder.mediaPolicy(MediaPolicy.HYBRID);
        newVolumeBuilder.dataProtectionPolicy(24, TimeUnit.HOURS);
        newVolumeBuilder.qosPolicy(new QosPolicy(1, 0, 0));
        Volume newVolume = newVolumeBuilder.create();

        String addedVolumeString;
        try
        {
            addedVolumeString = endpoint.doWriteThenRead(connection,
                                                         ObjectModelHelper.toJSON(newVolume),
                                                         StandardCharsets.UTF_8);
        }
        catch (HttpException e)
        {
            throw new ExecutionException(e);
        }

        if (listener instanceof WorkloadEventListener)
        {
            Volume addedVolume = ObjectModelHelper.toObject(addedVolumeString, Volume.class);
            
            ((WorkloadEventListener)listener).reportVolumeAdded(
                    _name,
                    VolumeQosSettings.fromVolume(addedVolume));
        }
    }

    @Override
	public URI getRelativeUri()
	{
		URI base = Fds.Api.V08.getBase();
		URI createVolume = Fds.Api.V08.getVolumes();
		
		return base.relativize(createVolume);
	}
	
	@Override
	public String getRequestMethod()
	{
	    return "POST";
	}
	
	@Override
	protected Stream<SimpleImmutableEntry<String, String>> toStringMembers()
	{
	    return Stream.concat(super.toStringMembers(),
	                         Stream.of(memberToString("name", _name)));
	}
	
	private final String _name;
}

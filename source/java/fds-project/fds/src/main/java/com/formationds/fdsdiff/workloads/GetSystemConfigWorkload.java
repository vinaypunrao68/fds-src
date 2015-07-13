package com.formationds.fdsdiff.workloads;

import static com.formationds.commons.util.Strings.javaString;

import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.function.Consumer;
import java.util.stream.Stream;

import com.formationds.client.v08.model.Tenant;
import com.formationds.iodriver.endpoints.OmEndpoint;
import com.formationds.iodriver.operations.GetTenants;
import com.formationds.iodriver.operations.Operation;
import com.formationds.iodriver.workloads.Workload;

public final class GetSystemConfigWorkload extends Workload
{
	public GetSystemConfigWorkload(boolean logOperations)
	{
		super(logOperations);
	}

    @Override
    public Class<?> getEndpointType()
    {
        return OmEndpoint.class;
    }
	
	@Override
	protected List<Stream<Operation>> createOperations()
	{
		Consumer<Collection<Tenant>> readTenants = ct ->
		{
			for (Tenant tenant : ct)
			{
				System.out.println("Tenant: " + javaString(tenant.toString()));
			}
		};
		
		return Collections.singletonList(Stream.of(new GetTenants(readTenants)));
	}
}

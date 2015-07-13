package com.formationds.fdsdiff.workloads;

import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.function.Consumer;
import java.util.stream.Stream;

import com.formationds.client.v08.model.Tenant;
import com.formationds.iodriver.endpoints.OrchestrationManagerEndpoint;
import com.formationds.iodriver.operations.GetTenants;
import com.formationds.iodriver.operations.OmOperation;
import com.formationds.iodriver.workloads.Workload;

import static com.formationds.commons.util.Strings.javaString;

public final class GetSystemConfigWorkload extends Workload<OrchestrationManagerEndpoint,
                                                            OmOperation>
{
	public GetSystemConfigWorkload(boolean logOperations)
	{
		super(OrchestrationManagerEndpoint.class,
		      OmOperation.class,
		      logOperations);
	}
	
	@Override
	protected List<Stream<OmOperation>> createOperations()
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

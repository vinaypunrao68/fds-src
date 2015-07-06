package com.formationds.fdsdiff.workloads;

import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.function.Consumer;
import java.util.stream.Stream;

import com.formationds.client.v08.model.Tenant;
import com.formationds.iodriver.endpoints.OrchestrationManagerEndpoint;
import com.formationds.iodriver.operations.GetTenants;
import com.formationds.iodriver.operations.OrchestrationManagerOperation;
import com.formationds.iodriver.workloads.Workload;

import static com.formationds.commons.util.Strings.javaString;

public final class GetSystemConfigWorkload extends Workload<OrchestrationManagerEndpoint,
                                                            OrchestrationManagerOperation>
{
	public GetSystemConfigWorkload(boolean logOperations)
	{
		super(OrchestrationManagerEndpoint.class,
		      OrchestrationManagerOperation.class,
		      logOperations);
	}
	
	@Override
	protected List<Stream<OrchestrationManagerOperation>> createOperations()
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

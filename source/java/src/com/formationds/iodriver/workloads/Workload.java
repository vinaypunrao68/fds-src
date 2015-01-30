package com.formationds.iodriver.workloads;

import static com.formationds.commons.util.ExceptionHelper.tunnel;

import java.util.stream.Stream;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.operations.ExecutionException;
import com.formationds.iodriver.operations.Operation;

public class Workload<EndpointT extends Endpoint<EndpointT, OperationT>, OperationT extends Operation<OperationT, EndpointT>>
{
    public Workload()
    {}

    public final void runOn(EndpointT endpoint) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");

        ensureInitialized();

        tunnel(ExecutionException.class,
               from -> _operations.forEach(from),
               (OperationT op) -> endpoint.doVisit(op));
    }

    public final void setUp(EndpointT endpoint) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");

        ensureInitialized();

        tunnel(ExecutionException.class,
               from -> _setup.forEach(from),
               (OperationT op) -> endpoint.doVisit(op));
    }

    public final void tearDown(EndpointT endpoint) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");

        ensureInitialized();

        tunnel(ExecutionException.class,
               from -> _teardown.forEach(from),
               (OperationT op) -> endpoint.doVisit(op));
    }

    protected Stream<OperationT> createOperations()
    {
        return Stream.empty();
    }

    protected Stream<OperationT> createSetup()
    {
        return Stream.empty();
    }

    protected Stream<OperationT> createTeardown()
    {
        return Stream.empty();
    }

    protected void ensureInitialized()
    {
        if (_operations == null)
        {
            _operations = createOperations();
            _setup = createSetup();
            _teardown = createTeardown();
        }
    }

    private Stream<OperationT> _operations;

    private Stream<OperationT> _setup;

    private Stream<OperationT> _teardown;
}

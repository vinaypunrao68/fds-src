package com.formationds.iodriver.workloads;

import java.util.stream.Stream;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.util.ExceptionHelper;
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

        // The type arguments can be inferred, so the call is just "tunnel(...)", but hits this
        // compiler bug:
        //
        // http://bugs.java.com/bugdatabase/view_bug.do?bug_id=8054210
        //
        // Linked Backports claim this is fixed in 8u40, however the above bug description claims
        // that it's still present in 8u40. 8u45 (another backport version) has not been tested,
        // but neither is currently in our apt repos.
        //
        // When this bug is fixed (check 8u40 and >= 8u45), reduce to just tunnel(...).
        ExceptionHelper.<OperationT, ExecutionException>tunnel(ExecutionException.class,
                                                               from -> _operations.forEach(from),
                                                               (OperationT op) -> endpoint.doVisit(op));
    }

    public final void setUp(EndpointT endpoint) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");

        ensureInitialized();

        // See comment in runOn().
        ExceptionHelper.<OperationT, ExecutionException>tunnel(ExecutionException.class,
                                                               from -> _setup.forEach(from),
                                                               (OperationT op) -> endpoint.doVisit(op));
    }

    public final void tearDown(EndpointT endpoint) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");

        ensureInitialized();

        // See comment in runOn().
        ExceptionHelper.<OperationT, ExecutionException>tunnel(ExecutionException.class,
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

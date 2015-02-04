package com.formationds.iodriver.workloads;

import static com.formationds.commons.util.ExceptionHelper.rethrowTunneled;
import static com.formationds.commons.util.ExceptionHelper.tunnel;
import static com.formationds.commons.util.ExceptionHelper.tunnelNow;

import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.concurrent.BrokenBarrierException;
import java.util.concurrent.CyclicBarrier;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.stream.Stream;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.TunneledException;
import com.formationds.commons.util.ExceptionHelper;
import com.formationds.commons.util.NullableMutableReference;
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

        Map<Thread, NullableMutableReference<Throwable>> workerThreads =
                new HashMap<>(_operations.size());
        CyclicBarrier startingGate = new CyclicBarrier(_operations.size());
        for (Stream<OperationT> work : _operations)
        {
            Thread workerThread =
                    new Thread(() ->
                    {
                        ExceptionHelper.<ExecutionException>tunnelNow(() -> awaitGate(startingGate),
                                                                      ExecutionException.class);

                        work.forEach(ExceptionHelper.<OperationT, ExecutionException>tunnel(op -> endpoint.doVisit(op),
                                                                                            ExecutionException.class));
                    });

            workerThread.setUncaughtExceptionHandler((thread, exception) ->
            {
                workerThreads.get(thread).set(exception);
            });

            workerThreads.put(workerThread, new NullableMutableReference<>());

            // The type arguments can be inferred, so the call is just "tunnel(...)", but hits
            // this compiler bug:
            //
            // http://bugs.java.com/bugdatabase/view_bug.do?bug_id=8054210
            //
            // Linked Backports claim this is fixed in 8u40, however the above bug description
            // claims that it's still present in 8u40. 8u45 (another backport version) has not
            // been tested, but neither is currently in our apt repos.
            //
            // When this bug is fixed (check 8u40 and >= 8u45), reduce to just tunnel(...).
        }

        workerThreads.keySet().forEach(t -> t.start());

        ExecutionException workerExceptions = null;
        for (Entry<Thread, NullableMutableReference<Throwable>> worker : workerThreads.entrySet())
        {
            Thread workerThread = worker.getKey();
            try
            {
                workerThread.join();
            }
            catch (InterruptedException e)
            {
                if (workerExceptions == null)
                {
                    workerExceptions = new ExecutionException("Error in worker thread(s).");
                }
                workerExceptions.addSuppressed(e);
            }

            Throwable error = worker.getValue().get();
            if (error != null)
            {
                if (error instanceof TunneledException)
                {
                    try
                    {
                        rethrowTunneled((TunneledException)error, ExecutionException.class);
                    }
                    catch (Throwable t)
                    {
                        error = t;
                    }
                }
                if (workerExceptions == null)
                {
                    workerExceptions = new ExecutionException("Error in worker thread(s).");
                }
                workerExceptions.addSuppressed(error);
            }
        }
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

    protected List<Stream<OperationT>> createOperations()
    {
        return Collections.emptyList();
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

    protected static void awaitGate(CyclicBarrier gate) throws ExecutionException
    {
        try
        {
            gate.await(1, TimeUnit.MINUTES);
        }
        catch (InterruptedException | BrokenBarrierException | TimeoutException e)
        {
            throw new ExecutionException(e);
        }
    }

    private List<Stream<OperationT>> _operations;

    private Stream<OperationT> _setup;

    private Stream<OperationT> _teardown;
}

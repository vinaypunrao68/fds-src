package com.formationds.iodriver.workloads;

import static com.formationds.commons.util.ExceptionHelper.getTunneledIfTunneled;

import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Optional;
import java.util.concurrent.BrokenBarrierException;
import java.util.concurrent.CyclicBarrier;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.function.Consumer;
import java.util.stream.Stream;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.util.ExceptionHelper;
import com.formationds.commons.util.NullableMutableReference;
import com.formationds.commons.util.functional.ExceptionThrowingConsumer;
import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.operations.Operation;
import com.formationds.iodriver.reporters.AbstractWorkloadEventListener;
import com.formationds.iodriver.validators.Validator;

/**
 * A basic skeleton for a workload.
 * 
 * The most important method to override is {@link #createOperations()}. {@link #createSetup()} and
 * {@link #createTeardown()} will also likely be overridden in most implementations.
 * 
 */
public abstract class Workload
{
    public abstract boolean doDryRun();
    
    public abstract Class<?> getEndpointType();
    
    public abstract Class<?> getListenerType();
    
    /**
     * Get a validator that will interpret this workload well.
     * 
     * @return The current property value, or {@link Optional#empty()} if there is no sensible
     *         default.
     */
    public Optional<Validator> getSuggestedValidator()
    {
        return Optional.empty();
    }
    
    /**
     * Execute this workload.
     * 
     * @param endpoint Operations will be run on this endpoint.
     * @param listener Operations will report to this listener.
     * 
     * @throws ExecutionException when an error occurs during execution of the workload.
     */
    // @eclipseFormat:off
    public final void runOn(Endpoint endpoint, AbstractWorkloadEventListener listener)
            throws ExecutionException
    // @eclipseFormat:on
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (listener == null) throw new NullArgumentException("listener");

        ensureOperationsInitialized();

        Map<Thread, NullableMutableReference<Throwable>> workerThreads =
                new HashMap<>(_operations.size());
        CyclicBarrier startingGate = new CyclicBarrier(_operations.size());
        for (Stream<Operation> work : _operations)
        {
            Thread workerThread =
                    new Thread(() ->
                    {
                        ExceptionThrowingConsumer<Operation, ExecutionException> exec =
                                op ->
                                {
                                    if (getLogOperations())
                                    {
                                        listener.reportOperationExecution(op);
                                    }
                                    endpoint.visit(op, listener);
                                };

                        // The type arguments can be inferred, so the call is just "tunnel(...)",
                        // but hits this compiler bug:
                        //
                        // http://bugs.java.com/bugdatabase/view_bug.do?bug_id=8054210
                        //
                        // Linked Backports claim this is fixed in 8u40, however the above bug
                        // description claims that it's still present in 8u40. 8u45 (another
                        // backport version) has not been tested, but neither is currently in our
                        // apt repos.
                        //
                        // When this bug is fixed (check 8u40 and >= 8u45), reduce to just
                        // tunnel(...) with a static import.
                        Consumer<Operation> tunneledExec =
                                ExceptionHelper.<Operation, ExecutionException>tunnel(
                                        exec, ExecutionException.class);
                        
                        // Synchronize all worker threads. This only ensures that they start work
                        // at the same time--if you need to synchronize at different points, use
                        // the AwaitGate operation.
                        ExceptionHelper.<ExecutionException>tunnelNow(() -> awaitGate(startingGate),
                                                                      ExecutionException.class);

                        // Execute each operation, tunneling any exceptions.
                        work.forEach(tunneledExec);
                    });

            // Store (suppress) exceptions from worker threads.
            workerThread.setUncaughtExceptionHandler((thread, exception) ->
            {
                // This really shouldn't be possible, but if it does happen, we know about it.
                if (thread == null) throw new NullArgumentException("thread");
                if (exception == null) throw new NullArgumentException("exception");

                Throwable untunneled = getTunneledIfTunneled(exception, ExecutionException.class);
                NullableMutableReference<Throwable> myErrorReference = workerThreads.get(thread);

                myErrorReference.set(untunneled);
            });

            workerThreads.put(workerThread, new NullableMutableReference<>());
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
                if (workerExceptions == null)
                {
                    workerExceptions = new ExecutionException("Error in worker thread(s).");
                }
                workerExceptions.addSuppressed(error);
            }
        }
        if (workerExceptions != null)
        {
        	throw workerExceptions;
        }
    }

    /**
     * Perform workflow setup, operations necessary to run but that do not have to be temporally
     * close to the body of the workflow.
     * 
     * @param endpoint The endpoint to run setup commands on.
     * @param listener The listener to report progress to.
     * 
     * @throws ExecutionException when there is an error running the setup commands.
     */
    // @eclipseFormat:off
    public void setUp(Endpoint endpoint,
                      AbstractWorkloadEventListener listener) throws ExecutionException
    // @eclipseFormat:on
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (listener == null) throw new NullArgumentException("listener");

        ensureSetupInitialized();

        // See comment in runOn().
        ExceptionHelper.<Operation, ExecutionException>tunnel(
                ExecutionException.class,
                from -> _setup.forEach(from),
                op ->
                {
                    if (getLogOperations())
                    {
                        listener.reportOperationExecution(op);
                    }
                    endpoint.visit(op, listener);
                });
    }

    /**
     * Perform workflow teardown, operations necessary to return {@code endpoint} to a clean state,
     * but that do not have to be temporally close to the body of the workflow.
     * 
     * @param endpoint The endpoint to run teardown commands on.
     * @param listener The listener to report progress to.
     * 
     * @throws ExecutionException when there is an error running the teardown commands.
     */
    // @eclipseFormat:off
    public final void tearDown(Endpoint endpoint,
                               AbstractWorkloadEventListener listener) throws ExecutionException
    // @eclipseFormat:on
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (listener == null) throw new NullArgumentException("listener");

        ensureTeardownInitialized();

        // See comment in runOn().
        ExceptionHelper.<Operation, ExecutionException>tunnel(
                ExecutionException.class,
                from -> _teardown.forEach(from),
                op ->
                {
                    if (getLogOperations())
                    {
                        listener.reportOperationExecution(op);
                    }
                    endpoint.visit(op, listener);
                });
    }

    /**
     * Constructor.
     * 
     * @param endpointType The type of endpoint this workload can use.
     * @param operationType The type of operation this workload can run.
     * @param logOperations Log operations executed by this workload.
     */
    protected Workload(boolean logOperations)
    {
        _logOperations = logOperations;
    }
    
    /**
     * Create the operations that will be executed.
     * 
     * This method should be overridden for all workloads.
     * 
     * @return A list of streams of operations, with the list of streams run in parallel.
     */
    protected List<Stream<Operation>> createOperations()
    {
        return Collections.emptyList();
    }

    /**
     * Create the operations that will prepare for a workload run.
     * 
     * @return A stream of operations.
     */
    protected Stream<Operation> createSetup()
    {
        return Stream.empty();
    }

    /**
     * Create the operations that will clean up a workload run.
     * 
     * @return A stream of operations.
     */
    protected Stream<Operation> createTeardown()
    {
        return Stream.empty();
    }

    protected final void ensureOperationsInitialized()
    {
        if (_operations == null)
        {
            _operations = createOperations();
        }
    }
    
    protected final void ensureSetupInitialized()
    {
        if (_setup == null)
        {
            _setup = createSetup();
        }
    }
    
    protected final void ensureTeardownInitialized()
    {
        if (_teardown == null)
        {
            _teardown = createTeardown();
        }
    }

    /**
     * Return whether to log operations executed by this workload.
     * 
     * @return The current property value.
     */
    protected final boolean getLogOperations()
    {
        return _logOperations;
    }
    
    /**
     * Wait to a gate to be released.
     * 
     * @param gate The gate to wait on.
     * 
     * @throws ExecutionException if an error occurs trying to wait.
     */
    protected static void awaitGate(CyclicBarrier gate) throws ExecutionException
    {
        if (gate == null) throw new NullArgumentException("gate");

        try
        {
            gate.await(1, TimeUnit.MINUTES);
        }
        catch (InterruptedException | BrokenBarrierException | TimeoutException e)
        {
            throw new ExecutionException(e);
        }
    }

    /**
     * Log operations executed by this workload.
     */
    private boolean _logOperations;
    
    /**
     * Workload body. Streams are run in parallel with each other.
     */
    private List<Stream<Operation>> _operations;

    /**
     * Setup instructions.
     */
    private Stream<Operation> _setup;

    /**
     * Cleanup instructions.
     */
    private Stream<Operation> _teardown;
}

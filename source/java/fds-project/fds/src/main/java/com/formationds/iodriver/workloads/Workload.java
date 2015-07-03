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
import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.logging.Logger;
import com.formationds.iodriver.operations.ExecutionException;
import com.formationds.iodriver.operations.Operation;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;
import com.formationds.iodriver.validators.Validator;

/**
 * A basic skeleton for a workload.
 * 
 * The most important method to override is {@link #createOperations()}. {@link #createSetup()} and
 * {@link #createTeardown()} will also likely be overridden in most implementations.
 * 
 * @param <EndpointT> The type of endpoint the workload will be run on.
 * @param <OperationT> The base type of operations in this workload.
 */
// @eclipseFormat:off
public abstract class Workload<EndpointT extends Endpoint<EndpointT, OperationT>,
                               OperationT extends Operation<OperationT, EndpointT>>
// @eclipseFormat:on
{
    /**
     * Get the type of endpoint this workload can use.
     * 
     * @return The current property value.
     */
    public final Class<EndpointT> getEndpointType()
    {
        return _endpointType;
    }

    /**
     * Get the type of operation this workload can execute.
     * 
     * @return The current property value.
     */
    public final Class<OperationT> getOperationType()
    {
        return _operationType;
    }
    
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
     * Constructor.
     * 
     * @param endpointType The type of endpoint this workload can use.
     * @param operationType The type of operation this workload can run.
     * @param logOperations Log operations executed by this workload.
     */
    protected Workload(Class<EndpointT> endpointType,
                       Class<OperationT> operationType,
                       boolean logOperations)
    {
        if (endpointType == null) throw new NullArgumentException("endpointType");
        if (operationType == null) throw new NullArgumentException("operationType");
        
        _endpointType = endpointType;
        _operationType = operationType;
        _logOperations = logOperations;
    }
    
    private ExceptionThrowingConsumer<OperationT, ExecutionException> debugWrap(
            ExceptionThrowingConsumer<OperationT, ExecutionException> exec, Logger logger)
    {
        if (exec == null) throw new NullArgumentException("exec");
        
        if (logger == null)
        {
            return exec;
        }
        else
        {
            return op -> 
                   {
                       logger.logDebug("Executing: " + op);
                       exec.accept(op);
                   };
        }
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
    public final void runOn(EndpointT endpoint, AbstractWorkflowEventListener listener)
            throws ExecutionException
    // @eclipseFormat:on
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (listener == null) throw new NullArgumentException("listener");

        ensureInitialized();

        Map<Thread, NullableMutableReference<Throwable>> workerThreads =
                new HashMap<>(_operations.size());
        CyclicBarrier startingGate = new CyclicBarrier(_operations.size());
        for (Stream<OperationT> work : _operations)
        {
            Thread workerThread =
                    new Thread(() ->
                    {
                        ExceptionThrowingConsumer<OperationT, ExecutionException> exec =
                                op -> endpoint.doVisit(op, listener);

                        // Log operations if requested.
                        ExceptionThrowingConsumer<OperationT, ExecutionException> loggingExec =
                                debugWrap(exec, getLogOperations() ? listener.getLogger() : null); 

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
                        Consumer<OperationT> tunneledExec =
                                ExceptionHelper.<OperationT, ExecutionException>
                                               tunnel(loggingExec, ExecutionException.class);
                        
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
    public final void setUp(EndpointT endpoint,
                            AbstractWorkflowEventListener listener) throws ExecutionException
    // @eclipseFormat:on
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (listener == null) throw new NullArgumentException("listener");

        ensureInitialized();

        // See comment in runOn().
        ExceptionHelper.<OperationT, ExecutionException>
                       tunnel(ExecutionException.class,
                              from -> _setup.forEach(from),
                              debugWrap((OperationT op) -> endpoint.doVisit(op, listener),
                                        getLogOperations() ? listener.getLogger() : null));
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
    public final void tearDown(EndpointT endpoint,
                               AbstractWorkflowEventListener listener) throws ExecutionException
    // @eclipseFormat:on
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (listener == null) throw new NullArgumentException("listener");

        ensureInitialized();

        // See comment in runOn().
        ExceptionHelper.<OperationT, ExecutionException>
                       tunnel(ExecutionException.class,
                              from -> _teardown.forEach(from),
                              debugWrap((OperationT op) -> endpoint.doVisit(op, listener),
                                        getLogOperations() ? listener.getLogger() : null));
    }

    /**
     * Create the operations that will be executed.
     * 
     * This method should be overridden for all workloads.
     * 
     * @return A list of streams of operations, with the list of streams run in parallel.
     */
    protected List<Stream<OperationT>> createOperations()
    {
        return Collections.emptyList();
    }

    /**
     * Create the operations that will prepare for a workload run.
     * 
     * @return A stream of operations.
     */
    protected Stream<OperationT> createSetup()
    {
        return Stream.empty();
    }

    /**
     * Create the operations that will clean up a workload run.
     * 
     * @return A stream of operations.
     */
    protected Stream<OperationT> createTeardown()
    {
        return Stream.empty();
    }

    /**
     * Ensure the workload is ready to run.
     */
    protected final void ensureInitialized()
    {
        if (_operations == null)
        {
            try
            {
                _operations = createOperations();
                _setup = createSetup();
                _teardown = createTeardown();
            }
            catch (Throwable t)
            {
                _operations = null;
                _setup = null;
                _teardown = null;

                throw t;
            }
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
     * The type of endpoint this workload can use.
     */
    private final Class<EndpointT> _endpointType;
    
    /**
     * Log operations executed by this workload.
     */
    private boolean _logOperations;
    
    /**
     * The type of operation this workload can run.
     */
    private final Class<OperationT> _operationType;

    /**
     * Workload body. Streams are run in parallel with each other.
     */
    private List<Stream<OperationT>> _operations;

    /**
     * Setup instructions.
     */
    private Stream<OperationT> _setup;

    /**
     * Cleanup instructions.
     */
    private Stream<OperationT> _teardown;
}

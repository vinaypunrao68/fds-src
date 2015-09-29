package com.formationds.iodriver;

import java.io.Closeable;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.reporters.WorkloadEventListener;
import com.formationds.iodriver.validators.Validator;
import com.formationds.iodriver.workloads.Workload;

/**
 * Coordinates executing a workload on an endpoint.
 */
public final class Driver
{
    /**
     * Constructor.
     * 
     * @param endpoint The endpoint to run {@code workload} on.
     * @param workload The workload to run on {@code endpoint}.
     * @param listener Receives events during the workload run.
     * @param validator Performs final check on the data {@code listener} gathered.
     */
    public Driver(Endpoint endpoint,
                  Workload workload,
                  WorkloadEventListener listener,
                  Validator validator)
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (workload == null) throw new NullArgumentException("workload");
        if (listener == null) throw new NullArgumentException("listener");
        if (validator == null) throw new NullArgumentException("validator");

        _context = null;
        _endpoint = endpoint;
        _listener = listener;
        _workload = workload;
        _validator = validator;
    }

    /**
     * Run the {@link #getWorkload() workload} setup if it hasn't already been run.
     * 
     * @throws ExecutionException when an error occurs running the workload setup.
     */
    public void ensureSetUp() throws ExecutionException
    {
        if (!_isSetUp)
        {
            WorkloadEventListener listener = getListener();
            
            _context = getValidator().newContext(listener);
            getWorkload().setUp(getEndpoint(), listener);
            _isSetUp = true;
        }
    }

    /**
     * Run the {@link #getWorkload() workload} teardown if it's currently set up.
     * 
     * @throws ExecutionException when an error occurs running the workload teardown.
     */
    public void ensureTearDown() throws ExecutionException
    {
        if (_isSetUp)
        {
            getWorkload().tearDown(getEndpoint(), getListener());
            _isSetUp = false;
        }
    }

    /**
     * Get the endpoint to run the {@link #getWorkload() workload} on.
     * 
     * @return The current property value.
     */
    public Endpoint getEndpoint()
    {
        return _endpoint;
    }

    /**
     * Get the event listener.
     * 
     * @return The current property value.
     */
    public WorkloadEventListener getListener()
    {
        return _listener;
    }

    /**
     * Get the result of running the workload from the {@link #getValidator() validator}.
     * 
     * @return 0 for success, other for failure.
     */
    public int getResult()
    {
        Validator validator = getValidator();

        if (validator.isValid(_context))
        {
            return 0;
        }
        else
        {
            return 1;
        }
    }

    /**
     * Get the validator used to decide whether this workload succeeded or failed based on data
     * collected by the {@link #getListener() listener}.
     * 
     * @return The current property value.
     */
    public Validator getValidator()
    {
        return _validator;
    }

    /**
     * Get the workload that is run on the {@link #getEndpoint() endpoint}.
     * 
     * @return The current property value.
     */
    public Workload getWorkload()
    {
        return _workload;
    }

    /**
     * Run the {@link #getWorkload() workload}.
     * 
     * @throws ExecutionException when an error occurs during execution.
     */
    public void runWorkload() throws ExecutionException
    {
        ensureSetUp();
        try
        {
            getWorkload().runOn(getEndpoint(), getListener());
        }
        finally
        {
            ensureTearDown();
        }
    }

    /**
     * Set the event listener.
     * 
     * @param listener The property value to set.
     */
    public void setListener(WorkloadEventListener listener)
    {
        if (listener == null) throw new NullArgumentException("listener");

        _listener = listener;
    }

    public static Driver newDriver(Endpoint endpoint,
                                   Workload workload,
                                   WorkloadEventListener listener,
                                   Validator validator)
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (workload == null) throw new NullArgumentException("workload");
        if (listener == null) throw new NullArgumentException("listener");
        if (validator == null) throw new NullArgumentException("validator");

        return new Driver(endpoint, workload, listener, validator);
    }
    
    private Closeable _context;
    
    /**
     * The service endpoint that {@link #_workload} is run on.
     */
    private final Endpoint _endpoint;

    /**
     * Whether {@link #_workload} has run its setup routine on {@link #_endpoint} without running
     * teardown after.
     */
    private boolean _isSetUp;

    /**
     * Observes events from {@link #_workload}.
     */
    private WorkloadEventListener _listener;

    /**
     * The instructions to run on {@link #_endpoint}.
     */
    private final Workload _workload;

    /**
     * Checks the data gathered by {@link #_listener} and determines if {@link #_workload} was
     * successful.
     */
    private final Validator _validator;
}

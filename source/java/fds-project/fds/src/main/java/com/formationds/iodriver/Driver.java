package com.formationds.iodriver;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.operations.ExecutionException;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;
import com.formationds.iodriver.validators.Validator;
import com.formationds.iodriver.workloads.Workload;

/**
 * Coordinates executing a workload on an endpoint.
 * 
 * @param <EndpointT> The type of endpoint to run the workload on.
 * @param <WorkloadT> The type of workload to run on the endpoint.
 */
// @eclipseFormat:off
public final class Driver<EndpointT extends Endpoint<EndpointT, ?>,
                          WorkloadT extends Workload<EndpointT, ?>>
// @eclipseFormat:on
{
    /**
     * Constructor.
     * 
     * @param endpoint The endpoint to run {@code workload} on.
     * @param workload The workload to run on {@code endpoint}.
     * @param listener Receives events during the workload run.
     * @param validator Performs final check on the data {@code listener} gathered.
     */
    public Driver(EndpointT endpoint,
                  WorkloadT workload,
                  AbstractWorkflowEventListener listener,
                  Validator validator)
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (workload == null) throw new NullArgumentException("workload");
        if (listener == null) throw new NullArgumentException("listener");
        if (validator == null) throw new NullArgumentException("validator");

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
            getWorkload().setUp(getEndpoint(), getListener());
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
    public EndpointT getEndpoint()
    {
        return _endpoint;
    }

    /**
     * Get the event listener.
     * 
     * @return The current property value.
     */
    public AbstractWorkflowEventListener getListener()
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
        AbstractWorkflowEventListener listener = getListener();
        Validator validator = getValidator();

        if (validator.isValid(listener))
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
    public WorkloadT getWorkload()
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
        AbstractWorkflowEventListener listener = getListener();
        try
        {
            getWorkload().runOn(getEndpoint(), listener);
        }
        finally
        {
            ensureTearDown();
        }
        listener.finished();
    }

    /**
     * Set the event listener.
     * 
     * @param listener The property value to set.
     */
    public void setListener(AbstractWorkflowEventListener listener)
    {
        if (listener == null) throw new NullArgumentException("listener");
        
        _listener = listener;
    }
    
    /**
     * The service endpoint that {@link #_workload} is run on.
     */
    private final EndpointT _endpoint;

    /**
     * Whether {@link #_workload} has run its setup routine on {@link #_endpoint} without running
     * teardown after.
     */
    private boolean _isSetUp;

    /**
     * Observes events from {@link #_workload}.
     */
    private AbstractWorkflowEventListener _listener;

    /**
     * The instructions to run on {@link #_endpoint}.
     */
    private final WorkloadT _workload;

    /**
     * Checks the data gathered by {@link #_listener} and determines if {@link #_workload} was
     * successful.
     */
    private final Validator _validator;
}

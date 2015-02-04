package com.formationds.iodriver;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.operations.ExecutionException;
import com.formationds.iodriver.reporters.QosValidator;
import com.formationds.iodriver.reporters.WorkflowEventListener;
import com.formationds.iodriver.workloads.Workload;

public final class Driver<EndpointT extends Endpoint<EndpointT, ?>, WorkloadT extends Workload<EndpointT, ?>>
{
    public Driver(EndpointT endpoint,
                  WorkloadT workload,
                  WorkflowEventListener listener,
                  QosValidator validator)
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

    public void ensureSetUp() throws ExecutionException
    {
        if (!_isSetUp)
        {
            getWorkload().setUp(getEndpoint(), getListener());
            _isSetUp = true;
        }
    }

    public void ensureTearDown() throws ExecutionException
    {
        if (_isSetUp)
        {
            getWorkload().tearDown(getEndpoint(), getListener());
            _isSetUp = false;
        }
    }

    public EndpointT getEndpoint()
    {
        return _endpoint;
    }

    public WorkflowEventListener getListener()
    {
        return _listener;
    }

    public int getResult()
    {
        WorkflowEventListener listener = getListener();
        QosValidator validator = getValidator();
        
        if (validator.wereIopsInRange(listener))
        {
            return 0;
        }
        else
        {
            return 1;
        }
    }

    public QosValidator getValidator()
    {
        return _validator;
    }
    
    public WorkloadT getWorkload()
    {
        return _workload;
    }

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

    private final EndpointT _endpoint;

    private boolean _isSetUp;

    private final WorkflowEventListener _listener;

    private final WorkloadT _workload;
    
    private final QosValidator _validator;
}

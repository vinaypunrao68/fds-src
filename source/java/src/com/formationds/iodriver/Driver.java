package com.formationds.iodriver;

public final class Driver
{
    public Driver()
    {
        _endpoint = Endpoint.LOCALHOST;
        _workload = Workload.NULL;
    }
    
    public final Endpoint getEndpoint()
    {
        return _endpoint;
    }
    
    public final Workload getWorkload()
    {
        return _workload;
    }
    
    public final void runWorkload()
    {
        getWorkload().runOn(getEndpoint());
    }
    
    private Endpoint _endpoint;
    
    private Workload _workload;
}

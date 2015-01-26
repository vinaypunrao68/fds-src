package com.formationds.iodriver;

import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.workloads.S3QosTestWorkload;

public final class Config
{
    public final static class Defaults
    {
        public static Endpoint getEndpoint()
        {
            return _endpoint;
        }
        
        public static Workload getWorkload()
        {
            return _workload;
        }
        
        static
        {
            _endpoint = Endpoint.LOCALHOST;
            _workload = new S3QosTestWorkload(4);
        }
        
        private Defaults()
        {
            throw new UnsupportedOperationException("Instantiating a utility class.");
        }
        
        private static final Endpoint _endpoint;
        
        private static final Workload _workload;
    }
    
    public Endpoint getEndpoint()
    {
        // TODO: Allow this to be specified.
        return Defaults.getEndpoint();
    }
    
    public Workload getWorkload()
    {
        // TODO: Allow this to be specified.
        return Defaults.getWorkload();
    }
}

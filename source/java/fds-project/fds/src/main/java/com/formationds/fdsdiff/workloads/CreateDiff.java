package com.formationds.fdsdiff.workloads;

import java.util.Collections;
import java.util.List;
import java.util.stream.Stream;

import com.formationds.iodriver.endpoints.FdsEndpoint;
import com.formationds.iodriver.operations.FdsOperation;
import com.formationds.iodriver.workloads.Workload;

public class CreateDiff extends Workload<FdsEndpoint, FdsOperation>
{
    protected CreateDiff(boolean logOperations) {
        super(FdsEndpoint.class, FdsOperation.class, logOperations);
    }
    
    @Override
    protected List<Stream<FdsOperation>> createOperations()
    {
        Stream<FdsOperation> retval = Stream.empty();
        
        retval = Stream.concat(retval, createReadVolumesOperations());
        
        return Collections.singletonList(retval);
    }
    
    private Stream<FdsOperation> createReadVolumesOperations()
    {
        Stream<FdsOperation> retval = Stream.empty();
        
        return retval;
    }
}

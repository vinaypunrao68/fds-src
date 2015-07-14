package com.formationds.fdsdiff.workloads;

import java.io.IOException;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.function.Consumer;
import java.util.stream.Stream;

import com.formationds.client.v08.model.Volume;
import com.formationds.commons.NullArgumentException;
import com.formationds.fdsdiff.operations.ListVolumes;
import com.formationds.iodriver.endpoints.FdsEndpoint;
import com.formationds.iodriver.operations.Operation;
import com.formationds.iodriver.workloads.Workload;

public class CreateDiff extends Workload
{
    @Override
    public Class<?> getEndpointType()
    {
        return FdsEndpoint.class;
    }
    
    protected CreateDiff(OutputStream output, boolean logOperations)
    {
        super(logOperations);
        
        if (output == null) throw new NullArgumentException("output");
        
        _output = output;
    }
    
    @Override
    protected List<Stream<Operation>> createOperations()
    {
        Stream<Operation> retval = Stream.empty();
        
        retval = Stream.concat(retval, createReadVolumesOperations());
        
        return Collections.singletonList(retval);
    }
    
    private Stream<Operation> createReadVolumesOperations()
    {
        Stream<Operation> retval = Stream.empty();
        
        Consumer<Collection<Volume>> volumesSetter = vl ->
        {
            for (Volume volume : vl)
            {
                try {
                    _output.write(("Volume: " + volume.getName() + "\n").getBytes(StandardCharsets.UTF_8));
                } catch (IOException e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
            }
            _volumes = vl;
        };
        
        Stream<Operation> listVolumes = Stream.of(new ListVolumes(volumesSetter));
        
        retval = Stream.concat(retval, listVolumes);
        
        return retval;
    }
    
    private final OutputStream _output;
    
    private Collection<Volume> _volumes;
}

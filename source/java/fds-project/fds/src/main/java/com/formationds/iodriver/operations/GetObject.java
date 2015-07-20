package com.formationds.iodriver.operations;

import static com.formationds.commons.util.Strings.javaString;

import java.io.IOException;
import java.util.function.Consumer;
import java.util.function.Supplier;

import com.amazonaws.services.s3.AmazonS3Client;
import com.amazonaws.services.s3.model.S3Object;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.model.ObjectManifest;
import com.formationds.iodriver.reporters.AbstractWorkloadEventListener;

public final class GetObject extends S3Operation
{
    public GetObject(String bucketName,
                     String key,
                     Supplier<ObjectManifest.Builder<?, ? extends ObjectManifest>> builderSupplier,
                     Consumer<ObjectManifest> setter)
    {
        if (bucketName == null) throw new NullArgumentException("bucketName");
        if (key == null) throw new NullArgumentException("key");
        if (builderSupplier == null) throw new NullArgumentException("builderSupplier");
        if (setter == null) throw new NullArgumentException("setter");
        
        _bucketName = bucketName;
        _key = key;
        _builderSupplier = builderSupplier;
        _setter = setter;
    }
    
    @Override
    public void accept(S3Endpoint endpoint,
                       AmazonS3Client client,
                       AbstractWorkloadEventListener listener) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (client == null) throw new NullArgumentException("client");
        if (listener == null) throw new NullArgumentException("listener");

        try (S3Object object = client.getObject(_bucketName, _key))
        {
            ObjectManifest.Builder<?, ? extends ObjectManifest> builder = _builderSupplier.get();
            builder.set(object);
            _setter.accept(builder.build());
        }
        catch (IOException e)
        {
            throw new ExecutionException("Error closing object "
                                         + javaString(_bucketName + ":" + _key) + ".",
                                         e);
        }
    }
    
    private final String _bucketName;

    private final Supplier<ObjectManifest.Builder<?, ? extends ObjectManifest>> _builderSupplier;
    
    private final String _key;
    
    private final Consumer<ObjectManifest> _setter; 
}

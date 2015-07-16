package com.formationds.iodriver.model;

import static com.formationds.commons.util.Strings.javaString;

import java.io.IOException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Arrays;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Objects;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.xml.bind.DatatypeConverter;

import com.amazonaws.services.s3.model.S3Object;
import com.formationds.commons.NullArgumentException;

public class ExtendedObjectManifest extends BasicObjectManifest
{
    public static class Builder<ThisT extends Builder<ThisT>>
            extends BasicObjectManifest.Builder<ThisT>
    {
        public Builder() { }
        
        public Builder(ObjectManifest source)
        {
            super(source);
        }
        
        @Override
        public ExtendedObjectManifest build()
        {
            validate();
            
            return new ExtendedObjectManifest(getThis());
        }
        
        public final Map<String, String> getMetadata()
        {
            return _metadata;
        }
        
        public final byte[] getSha512()
        {
            return _sha512;
        }
        
        public final ThisT setMetadata(Map<String, String> value)
        {
            _metadata = value;
            return getThis();
        }
        
        public final ThisT setSha512(byte[] value)
        {
            if (value != null && value.length != 64)
            {
                throw new IllegalArgumentException("sha512 must be 64 bytes.");
            }
            
            _sha512 = value;
            return getThis();
        }
        
        @Override
        protected void setInternal(S3Object object) throws IOException
        {
            if (object == null) throw new NullArgumentException("object");
            
            super.setInternal(object);

            setMetadata(object.getObjectMetadata().getUserMetadata());
        }
        
        @Override
        protected void streamContent(S3Object object) throws IOException
        {
            if (_sha512 == null)
            {
                try
                {
                    _sha512Summer = MessageDigest.getInstance("SHA-512");
                }
                catch (NoSuchAlgorithmException e)
                {
                    throw new IllegalStateException("SHA-512 is not a supported algorithm.");
                }
            }
            
            super.streamContent(object);
        }
        
        @Override
        protected void streamContent(byte[] nextChunk, int length)
        {
            if (nextChunk == null) throw new NullArgumentException("nextChunk");
            if (length < 0 || length > nextChunk.length)
            {
                throw new IllegalArgumentException("length " + length + " is invalid.");
            }

            super.streamContent(nextChunk, length);
            
            if (_sha512 == null)
            {
                _sha512Summer.update(nextChunk, 0, length);
            }
        }
        
        @Override
        protected void streamContentEnd()
        {
            super.streamContentEnd();
            
            if (_sha512 == null)
            {
                if (_sha512Summer == null)
                {
                    throw new IllegalStateException("_sha512Summer is null.");
                }
                _sha512 = _sha512Summer.digest();
            }
            _sha512Summer = null;
        }
        
        @Override
        protected void validate()
        {
            super.validate();
            
            if (_metadata == null) throw new IllegalStateException("metadata cannot be null.");
            if (_sha512 == null) throw new IllegalStateException("sha512 cannot be null.");
        }
        
        private Map<String, String> _metadata;
        
        private byte[] _sha512;
        
        private MessageDigest _sha512Summer;
    }
    
    @Override
    public boolean equals(Object other)
    {
        if (!super.equals(other))
        {
            return false;
        }
        
        ExtendedObjectManifest typedOther = (ExtendedObjectManifest)other;
        return Objects.equals(_metadata, typedOther._metadata)
               && Arrays.equals(_sha512, typedOther._sha512);
    }
    
    public final Map<String, String> getMetadata()
    {
        return _metadata;
    }
    
    @Override
    public int hashCode()
    {
        int hash = super.hashCode();
        
        hash = hash * 23 ^ _metadata.hashCode();
        hash = hash * 23 ^ Arrays.hashCode(_sha512);
        
        return hash;
    }
    
    protected ExtendedObjectManifest(Builder<?> builder)
    {
        super(builder);
        
        if (builder == null) throw new NullArgumentException("builder");
        
        _metadata = builder.getMetadata();
        _sha512 = builder.getSha512();
    }
    
    @Override
    protected void setBuilderProperties(BasicObjectManifest.Builder<?> builder)
    {
        if (builder == null) throw new NullArgumentException("builder");
        
        if (builder instanceof Builder)
        {
            setBuilderProperties((Builder<?>)builder);
        }
        else
        {
            super.setBuilderProperties(builder);
        }
    }
    
    protected void setBuilderProperties(Builder<?> builder)
    {
        if (builder == null) throw new NullArgumentException("builder");
        
        super.setBuilderProperties(builder);
        
        builder.setMetadata(getMetadata());
    }
    
    @Override
    protected Stream<Entry<String, String>> toJsonStringMembers()
    {
        return Stream.concat(
                super.toJsonStringMembers(),
                Stream.of(
                        memberToJsonString(
                                "metadata",
                                "{ "
                                + _metadata.entrySet()
                                           .stream()
                                           .map(kvp -> javaString(kvp.getKey())
                                                       + ": " + javaString(kvp.getValue()))
                                           .collect(Collectors.joining(", "))
                                + " }"),
                        memberToJsonString(
                                "sha512",
                                javaString(DatatypeConverter.printBase64Binary(_sha512)))));
    }
    
    @Override
    protected Stream<Entry<String, String>> toStringMembers()
    {
        return Stream.concat(
                super.toStringMembers(),
                Stream.of(memberToString("metadata", _metadata),
                          memberToString("sha512", DatatypeConverter.printBase64Binary(_sha512))));
    }
    
    private final Map<String, String> _metadata;
    
    private final byte[] _sha512;
}

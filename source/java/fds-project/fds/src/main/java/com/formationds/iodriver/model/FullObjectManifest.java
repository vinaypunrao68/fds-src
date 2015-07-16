package com.formationds.iodriver.model;

import static com.formationds.commons.util.Strings.javaString;

import java.io.IOException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Arrays;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Optional;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.xml.bind.DatatypeConverter;

import com.amazonaws.services.s3.model.S3Object;
import com.amazonaws.services.s3.model.S3ObjectInputStream;
import com.formationds.commons.NullArgumentException;
import com.google.common.io.ByteStreams;

public class FullObjectManifest extends BasicObjectManifest
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
        public FullObjectManifest build()
        {
            validate();
            
            return new FullObjectManifest(getThis());
        }
        
        public final byte[] getContent()
        {
            return _content;
        }
        
        public final Map<String, String> getMetadata()
        {
            return _metadata;
        }
        
        @Override
        public ThisT set(S3Object object) throws IOException
        {
            if (object == null) throw new NullArgumentException("object");
            
            super.set(object);
            
            setMetadata(object.getObjectMetadata().getUserMetadata());
            try (S3ObjectInputStream contentStream = object.getObjectContent())
            {
                setContent(ByteStreams.toByteArray(contentStream));
            }
            return getThis();
        }
        
        public final ThisT setContent(byte[] value)
        {
            _content = value;
            return getThis();
        }
        
        public final ThisT setMetadata(Map<String, String> value)
        {
            _metadata = value;
            return getThis();
        }
        
        @Override
        protected void validate()
        {
            super.validate();
            
            if (_content == null) throw new IllegalStateException("content cannot be null.");
            
            Optional<byte[]> builderMd5Wrapper = getMd5();
            if (builderMd5Wrapper.isPresent())
            {
                MessageDigest md5Summer;
                try
                {
                    md5Summer = MessageDigest.getInstance("MD5");
                }
                catch (NoSuchAlgorithmException e)
                {
                    throw new IllegalStateException("MD5 algorithm is not supported.", e);
                }
                
                byte[] builderMd5 = builderMd5Wrapper.get();
                byte[] actualMd5 = md5Summer.digest(_content);
                if (!Arrays.equals(actualMd5, builderMd5))
                {
                    throw new IllegalStateException(
                            "MD5 provided does not match content: "
                            + "Actual(" + DatatypeConverter.printHexBinary(actualMd5) + "), "
                            + "Provided(" + DatatypeConverter.printHexBinary(builderMd5) + ").");
                }
            }
            
            if (_metadata == null)
            {
                throw new IllegalStateException("metadata cannot be null.");
            }
        }
        
        private byte[] _content;
        
        private Map<String, String> _metadata;
    }
    
    @Override
    public boolean equals(Object other)
    {
        if (!super.equals(other))
        {
            return false;
        }
        
        FullObjectManifest typedOther = (FullObjectManifest)other;
        return Arrays.equals(_content, typedOther._content);
    }
    
    public final byte[] getContent()
    {
        return _content;
    }
    
    public final Map<String, String> getMetadata()
    {
        return _metadata;
    }
    
    @Override
    public int hashCode()
    {
        int hash = super.hashCode();
        
        hash = hash * 23 ^ Arrays.hashCode(_content);
        
        return hash;
    }
    
    protected FullObjectManifest(Builder<?> builder)
    {
        super(builder);
        
        if (builder == null) throw new NullArgumentException("builder");
        
        _content = builder.getContent();
        _metadata = builder.getMetadata();
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
        
        builder.setContent(getContent());
    }
    
    @Override
    protected Stream<Entry<String, String>> toJsonStringMembers()
    {
        return Stream.concat(
                super.toJsonStringMembers(),
                Stream.of(
                        memberToJsonString(
                                "content",
                                javaString(DatatypeConverter.printBase64Binary(_content))),
                        memberToJsonString(
                                "metadata",
                                "{ "
                                + _metadata.entrySet()
                                           .stream()
                                           .map(kvp -> javaString(kvp.getKey())
                                                       + ": " + javaString(kvp.getValue()))
                                           .collect(Collectors.joining(", "))
                                + " }"
                                )));
    }
    
    @Override
    protected Stream<Entry<String, String>> toStringMembers()
    {
        return Stream.concat(
                super.toStringMembers(),
                Stream.of(memberToString("content",
                                         DatatypeConverter.printBase64Binary(_content))));
    }
    
    private final byte[] _content;
    
    private final Map<String, String> _metadata;
}

package com.formationds.iodriver.model;

import static com.formationds.commons.util.Strings.javaString;

import java.io.IOException;
import java.time.ZoneId;
import java.time.ZonedDateTime;
import java.util.Arrays;
import java.util.Map.Entry;
import java.util.Optional;
import java.util.stream.Stream;

import javax.xml.bind.DatatypeConverter;

import com.amazonaws.services.s3.model.ObjectMetadata;
import com.amazonaws.services.s3.model.S3Object;
import com.formationds.commons.NullArgumentException;
import com.google.common.base.Objects;

public class BasicObjectManifest extends ObjectManifest
{
    public static class Builder<ThisT extends Builder<ThisT>> extends ObjectManifest.Builder<ThisT>
    {
        public Builder() { }
        
        public Builder(ObjectManifest source)
        {
            super(source);
        }
        
        @Override
        public BasicObjectManifest build()
        {
            validate();
            
            return new BasicObjectManifest(getThis());
        }
        
        public final Optional<ZonedDateTime> getLastModified()
        {
            return _lastModified;
        }
        
        public final Optional<byte[]> getMd5()
        {
            return _md5;
        }
        
        public final Long getSize()
        {
            return _size;
        }
        
        @Override
        public ThisT set(S3Object object) throws IOException
        {
            if (object == null) throw new NullArgumentException("object");

            super.set(object);
            
            ObjectMetadata metadata = object.getObjectMetadata();
            setLastModified(Optional.ofNullable(
                    metadata.getLastModified()).map(d -> d.toInstant().atZone(_PACIFIC)));
            setMd5(_md5 = Optional.ofNullable(
                    metadata.getContentMD5()).map(md5 -> DatatypeConverter.parseBase64Binary(md5)));
            setSize(_size = metadata.getContentLength());
            return getThis();
        }
        
        public final ThisT setLastModified(Optional<ZonedDateTime> value)
        {
            _lastModified = value;
            return getThis();
        }
        
        public final ThisT setMd5(Optional<byte[]> value)
        {
            if (value != null && value.isPresent() && value.get().length != 16)
            {
                throw new IllegalArgumentException("md5 must be a 128-bit (16-byte) MD5 sum.");
            }
            
            _md5 = value;
            return getThis();
        }
        
        public final ThisT setSize(Long value)
        {
            if (value != null && value < 0)
            {
                throw new IllegalArgumentException("size cannot be < 0.");
            }
            
            _size = value;
            return getThis();
        }
        
        @Override
        protected void validate()
        {
            super.validate();
            
            if (_lastModified == null) throw new IllegalStateException("lastModified cannot be null.");
            if (_md5 == null) throw new IllegalStateException("md5 cannot be null.");
            if (_size == null) throw new IllegalStateException("size cannot be null.");
        }
        
        private Optional<ZonedDateTime> _lastModified;
        
        private Optional<byte[]> _md5;
        
        private Long _size;
    }
    
    @Override
    public boolean equals(Object other)
    {
        if (!super.equals(other))
        {
            return false;
        }
        
        BasicObjectManifest typedOther = (BasicObjectManifest)other;
        return Objects.equal(_lastModified.orElse(null), typedOther._lastModified.orElse(null))
               && Arrays.equals(_md5.orElse(null), typedOther._md5.orElse(null))
               && _size == typedOther._size;
    }
    
    public final Optional<ZonedDateTime> getLastModified()
    {
        return _lastModified;
    }
    
    public final Optional<byte[]> getMd5()
    {
        return _md5;
    }
    
    public final long getSize()
    {
        return _size;
    }
    
    @Override
    public int hashCode()
    {
        int hash = super.hashCode();
        
        hash = hash * 23 ^ (_lastModified.isPresent() ? _lastModified.get().hashCode() : 0);
        hash = hash * 23 ^ (_md5.isPresent() ? Arrays.hashCode(_md5.get()) : 0);
        hash = hash * 23 ^ (int)(_size ^ (_size >>> 32));
        
        return hash;
    }
    
    protected BasicObjectManifest(Builder<?> builder)
    {
        super(builder);
        
        if (builder == null) throw new NullArgumentException("builder");
        
        _lastModified = builder.getLastModified();
        _md5 = builder.getMd5();
        _size = builder.getSize();
    }
    
    @Override
    protected void setBuilderProperties(ObjectManifest.Builder<?> builder)
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
        
        builder.setLastModified(getLastModified());
        builder.setMd5(getMd5());
        builder.setSize(getSize());
    }
    
    @Override
    protected Stream<Entry<String, String>> toJsonStringMembers()
    {
        return Stream.concat(
                super.toJsonStringMembers(),
                Stream.of(memberToJsonString("lastModified",
                                             _lastModified.map(lm -> javaString(lm.toString()))
                                                          .orElse("null")),
                          memberToJsonString(
                                  "md5",
                                  _md5.map(md5 -> javaString(
                                          DatatypeConverter.printBase64Binary(md5)))
                                      .orElse("null")),
                          memberToJsonString("size", Long.toString(_size))));
    }
    
    @Override
    protected Stream<Entry<String, String>> toStringMembers()
    {
        return Stream.concat(
                super.toStringMembers(),
                Stream.of(memberToString("lastModified",
                                         _lastModified.map(lm -> lm.toString())
                                                      .orElse(null)),
                          memberToString("md5",
                                         _md5.map(md5 -> DatatypeConverter.printBase64Binary(md5))
                                             .orElse(null)),
                          memberToString("size", _size)));
    }
    
    static
    {
        _PACIFIC = ZoneId.of("America/Los_Angeles");
    }
    
    private final Optional<ZonedDateTime> _lastModified;
    
    private final Optional<byte[]> _md5;
    
    private final long _size;
    
    private final static ZoneId _PACIFIC;
}

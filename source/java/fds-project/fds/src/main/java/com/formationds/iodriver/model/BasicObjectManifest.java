package com.formationds.iodriver.model;

import static com.formationds.commons.util.Strings.javaString;

import java.io.IOException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.time.ZoneId;
import java.time.ZonedDateTime;
import java.util.Arrays;
import java.util.Map.Entry;
import java.util.Optional;
import java.util.stream.Stream;

import javax.xml.bind.DatatypeConverter;

import com.amazonaws.services.s3.model.ObjectMetadata;
import com.amazonaws.services.s3.model.S3Object;
import com.amazonaws.services.s3.model.S3ObjectInputStream;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.model.ObjectManifest.GsonAdapter;
import com.google.common.base.Objects;
import com.google.gson.TypeAdapter;
import com.google.gson.annotations.JsonAdapter;
import com.google.gson.stream.JsonReader;
import com.google.gson.stream.JsonWriter;

@JsonAdapter(GsonAdapter.class)
public class BasicObjectManifest extends ObjectManifest
{
    public static class Builder<ThisT extends Builder<ThisT>> extends ObjectManifest.Builder<ThisT>
    {
        public Builder()
        {
            _md5Summer = null;
        }
        
        public Builder(ObjectManifest source)
        {
            super(source);
            
            _md5Summer = null;
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
        
        public final byte[] getMd5()
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

            setInternal(object);
            streamContent(object);
            return getThis();
        }
        
        public final ThisT setLastModified(Optional<ZonedDateTime> value)
        {
            _lastModified = value;
            return getThis();
        }
        
        public final ThisT setMd5(byte[] value)
        {
            if (value != null && value.length != 16)
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
        
        protected void setInternal(S3Object object) throws IOException
        {
            if (object == null) throw new NullArgumentException("object");
            
            super.set(object);
            
            ObjectMetadata metadata = object.getObjectMetadata();
            setLastModified(Optional.ofNullable(
                    metadata.getLastModified()).map(d -> d.toInstant().atZone(_PACIFIC)));
            String contentMd5 = metadata.getContentMD5();
            setMd5(_md5 = contentMd5 == null
                          ? null
                          : DatatypeConverter.parseBase64Binary(contentMd5));
            setSize(_size = metadata.getContentLength());
        }
        
        protected void streamContent(S3Object object) throws IOException
        {
            if (object == null) throw new NullArgumentException("object");
            
            if (_size == null)
            {
                _newSize = 0L;
            }
            if (_md5 == null)
            {
                try
                {
                    _md5Summer = MessageDigest.getInstance("MD5");
                }
                catch (NoSuchAlgorithmException e)
                {
                    throw new IllegalArgumentException("MD5 is not a supported algorithm.");
                }
            }
            
            try (S3ObjectInputStream contentStream = object.getObjectContent())
            {
                byte[] buffer = new byte[4096];
                int bytesRead = 0;
                while ((bytesRead = contentStream.read(buffer, 0, buffer.length)) > 0)
                {
                    streamContent(buffer, bytesRead);
                }
            }
            
            streamContentEnd();
        }
        
        protected void streamContent(byte[] nextChunk, int length)
        {
            if (nextChunk == null) throw new NullArgumentException("nextChunk");
            if (length < 0 || length > nextChunk.length)
            {
                throw new IllegalArgumentException("length " + length + " is invalid.");
            }
            
            // We don't get here unless the object is at least 1 byte.
            if (_size != null && _size == 0)
            {
                _size = null;
                _newSize = 0L;
            }
            if (_size == null)
            {
                _newSize += length;
            }
            if (_md5 == null)
            {
                _md5Summer.update(nextChunk, 0, length);
            }
        }
        
        protected void streamContentEnd()
        {
            if (_size == null)
            {
                _size = _newSize;
                _newSize = null;
            }
            if (_md5 == null)
            {
                _md5 = _md5Summer.digest();
            }
            _md5Summer = null;
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
        
        private byte[] _md5;
        
        private Long _size;
        
        private MessageDigest _md5Summer;
        
        private Long _newSize;
    }
    
    public static class GsonAdapter extends TypeAdapter<BasicObjectManifest>
    {
        @Override
        public BasicObjectManifest read(JsonReader in) throws IOException
        {
            if (in == null) throw new NullArgumentException("in");
            
            Builder<?> builder = new Builder<>();
            
            
        }
        
        @Override
        public void write(JsonWriter out, BasicObjectManifest source) throws IOException
        {
            if (out == null) throw new NullArgumentExcepiton("out");
        }
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
               && Arrays.equals(_md5, typedOther._md5)
               && _size == typedOther._size;
    }
    
    public final Optional<ZonedDateTime> getLastModified()
    {
        return _lastModified;
    }
    
    public final byte[] getMd5()
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
        hash = hash * 23 ^ Arrays.hashCode(_md5);
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
                                  javaString(DatatypeConverter.printBase64Binary(_md5))),
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
                                         DatatypeConverter.printBase64Binary(_md5)),
                          memberToString("size", _size)));
    }
    
    static
    {
        _PACIFIC = ZoneId.of("America/Los_Angeles");
    }
    
    private final Optional<ZonedDateTime> _lastModified;
    
    private final byte[] _md5;
    
    private final long _size;
    
    private final static ZoneId _PACIFIC;
}

package com.formationds.fdsdiff;

import static com.formationds.commons.util.Strings.javaString;

import java.time.ZonedDateTime;
import java.util.Arrays;
import java.util.Map.Entry;
import java.util.stream.Stream;

import javax.xml.bind.DatatypeConverter;

import com.formationds.commons.NullArgumentException;
import com.google.common.base.Objects;

public class BasicObjectManifest extends ObjectManifest
{
    public static class Builder<ThisT extends Builder<ThisT>> extends ObjectManifest.Builder<ThisT>
    {
        @Override
        public BasicObjectManifest build()
        {
            validate();
            
            return new BasicObjectManifest(getThis());
        }
        
        public final ZonedDateTime getLastModified()
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
        
        public final ThisT setLastModified(ZonedDateTime value)
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
        
        @Override
        protected void validate()
        {
            super.validate();
            
            if (_lastModified == null) throw new IllegalStateException("lastModified cannot be null.");
            if (_md5 == null) throw new IllegalStateException("md5 cannot be null.");
            if (_size == null) throw new IllegalStateException("size cannot be null.");
        }
        
        private ZonedDateTime _lastModified;
        
        private byte[] _md5;
        
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
        return Objects.equal(_lastModified, typedOther._lastModified)
               && Arrays.equals(_md5, typedOther._md5)
               && _size == typedOther._size;
    }
    
    @Override
    public int hashCode()
    {
        int hash = super.hashCode();
        
        hash = hash * 23 ^ (_lastModified == null ? 0 : _lastModified.hashCode());
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
    protected Stream<Entry<String, String>> toJsonStringMembers()
    {
        return Stream.concat(
                super.toJsonStringMembers(),
                Stream.of(memberToJsonString("lastModified", javaString(_lastModified.toString())),
                          memberToJsonString(
                                  "md5",
                                  javaString(DatatypeConverter.printBase64Binary(_md5))),
                          memberToJsonString("size", Long.toString(_size))));
    }
    
    @Override
    protected Stream<Entry<String, String>> toStringMembers()
    {
        return Stream.concat(super.toStringMembers(),
                             Stream.of(memberToString("lastModified", _lastModified),
                                       memberToString("md5",
                                                      DatatypeConverter.printBase64Binary(_md5)),
                                       memberToString("size", _size)));
    }
    
    private final ZonedDateTime _lastModified;
    
    private final byte[] _md5;
    
    private final long _size;
}

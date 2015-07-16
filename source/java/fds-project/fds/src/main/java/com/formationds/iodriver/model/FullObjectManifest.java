package com.formationds.iodriver.model;

import static com.formationds.commons.util.Strings.javaString;

import java.io.IOException;
import java.util.Arrays;
import java.util.Map.Entry;
import java.util.stream.Stream;

import javax.xml.bind.DatatypeConverter;

import com.amazonaws.services.s3.model.S3Object;
import com.formationds.commons.NullArgumentException;

public class FullObjectManifest extends ExtendedObjectManifest
{
    public static class Builder<ThisT extends Builder<ThisT>>
            extends ExtendedObjectManifest.Builder<ThisT>
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
        
        public final ThisT setContent(byte[] value)
        {
            _content = value;
            return getThis();
        }
        
        @Override
        protected void streamContent(S3Object object) throws IOException
        {
            if (_content == null)
            {
                _newContent = null;
                _newContentLength = 0;
                _ensureNewContentLength();
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
            
            if (_content == null)
            {
                int originalNewContentLength = 0;
                if (_newContent == null)
                {
                    _newContentLength = length;
                }
                else
                {
                    originalNewContentLength = _newContentLength;
                    _newContentLength += length;
                }
                _ensureNewContentLength();
                System.arraycopy(nextChunk, 0, _newContent, originalNewContentLength, length);
            }
        }
        
        @Override
        protected void streamContentEnd()
        {
            super.streamContentEnd();
            
            if (_content == null)
            {
                if (_newContent == null)
                {
                    throw new IllegalStateException("_newContent is null.");
                }
                _content = Arrays.copyOf(_newContent, _newContentLength);
            }
            _newContent = null;
        }
        
        @Override
        protected void validate()
        {
            super.validate();
            
            if (_content == null) throw new IllegalStateException("content cannot be null.");
        }
        
        private byte[] _content;
        
        private byte[] _newContent;
        
        private int _newContentLength;
        
        private void _ensureNewContentLength()
        {
            if (_newContent == null)
            {
                _newContent = new byte[4096];
            }
            
            int resizeTarget = _newContent.length;
            while (resizeTarget < _newContentLength)
            {
                resizeTarget *= 2;
            }
            if (resizeTarget != _newContent.length)
            {
                _newContent = Arrays.copyOf(_newContent, resizeTarget);
            }
        }
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
    }
    
    @Override
    protected void setBuilderProperties(ExtendedObjectManifest.Builder<?> builder)
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
                                javaString(DatatypeConverter.printBase64Binary(_content)))));
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
}

package com.formationds.iodriver.model;

import static com.formationds.commons.util.Strings.javaString;

import java.io.IOException;
import java.util.Arrays;
import java.util.Map.Entry;
import java.util.stream.Stream;

import javax.xml.bind.DatatypeConverter;

import com.amazonaws.services.s3.model.S3Object;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.model.FullObjectManifest.ConcreteGsonAdapter;
import com.google.gson.annotations.JsonAdapter;
import com.google.gson.stream.JsonReader;
import com.google.gson.stream.JsonWriter;

@JsonAdapter(ConcreteGsonAdapter.class)
public class FullObjectManifest extends ExtendedObjectManifest
{
    public static class Builder<ThisT extends Builder<ThisT, BuiltT>,
                                   BuiltT extends FullObjectManifest>
            extends ExtendedObjectManifest.Builder<ThisT, BuiltT>
    {
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
        
        public Builder() { }
        
        public Builder(BuiltT source)
        {
            super(source);
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

    public final static class ConcreteBuilder extends Builder<ConcreteBuilder,
                                                              FullObjectManifest>
    { }
    
    public final static class ConcreteGsonAdapter extends GsonAdapter<FullObjectManifest,
                                                                      ConcreteBuilder>
    {
        @Override
        protected FullObjectManifest build(ConcreteBuilder builder)
        {
            if (builder == null) throw new NullArgumentException("builder");
            
            return builder.build();
        }
        
        @Override
        protected ConcreteBuilder newContext()
        {
            return new ConcreteBuilder();
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
    public ComparisonDataFormat getFormat()
    {
        return ComparisonDataFormat.FULL;
    }
    
    @Override
    public int hashCode()
    {
        int hash = super.hashCode();
        
        hash = hash * 23 ^ Arrays.hashCode(_content);
        
        return hash;
    }
    
    protected static abstract class GsonAdapter<T extends FullObjectManifest,
                                                BuilderT extends Builder<?, T>>
            extends ExtendedObjectManifest.GsonAdapter<T, BuilderT>
    {
        @Override
        protected void readValue(String name, JsonReader in, BuilderT builder) throws IOException
        {
            if (name == null) throw new NullArgumentException("name");
            if (in == null) throw new NullArgumentException("in");
            if (builder == null) throw new NullArgumentException("builder");

            switch (name)
            {
            case "content":
                builder.setContent(DatatypeConverter.parseBase64Binary(in.nextString()));
                break;
            default:
                super.readValue(name, in, builder);
            }
        }
        
        @Override
        protected void writeValues(T source, JsonWriter out) throws IOException
        {
            if (source == null) throw new NullArgumentException("source");
            if (out == null) throw new NullArgumentException("out");

            super.writeValues(source, out);

            out.name("content");
            out.value(DatatypeConverter.printBase64Binary(source.getContent()));
        }
    }

    protected FullObjectManifest(Builder<?, ? extends FullObjectManifest> builder)
    {
        super(builder);
        
        if (builder == null) throw new NullArgumentException("builder");
        
        _content = builder.getContent();
    }
    
    @Override
    protected void setBuilderProperties(
            ExtendedObjectManifest.Builder<?, ? extends ExtendedObjectManifest> builder)
    {
        if (builder == null) throw new NullArgumentException("builder");
        
        if (builder instanceof Builder)
        {
            // We know this is safe because these are the constraints set on Builder<>.
            @SuppressWarnings("unchecked")
            Builder<?, ? extends FullObjectManifest> typedBuilder =
                    (Builder<?, ? extends FullObjectManifest>)builder;
            setBuilderProperties(typedBuilder);
        }
        else
        {
            super.setBuilderProperties(builder);
        }
    }
    
    protected void setBuilderProperties(Builder<?, ? extends FullObjectManifest> builder)
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

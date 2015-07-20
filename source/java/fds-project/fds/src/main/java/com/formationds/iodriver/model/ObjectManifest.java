package com.formationds.iodriver.model;

import static com.formationds.commons.util.Strings.javaString;

import java.io.IOException;
import java.util.AbstractMap.SimpleImmutableEntry;
import java.util.Map.Entry;
import java.util.Objects;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import com.amazonaws.services.s3.model.S3Object;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.model.ObjectManifest.ConcreteGsonAdapter;
import com.google.gson.annotations.JsonAdapter;
import com.google.gson.stream.JsonReader;
import com.google.gson.stream.JsonWriter;

@JsonAdapter(ConcreteGsonAdapter.class)
public class ObjectManifest
{
    public static class Builder<ThisT extends Builder<ThisT, BuiltT>,
                                   BuiltT extends ObjectManifest>
    {
        public ObjectManifest build()
        {
            validate();
            
            return new ObjectManifest(getThis());
        }
        
        public final String getName()
        {
            return _name;
        }
        
        public ThisT set(S3Object object) throws IOException
        {
            if (object == null) throw new NullArgumentException("object");
            
            setName(object.getKey());
            return getThis();
        }
        
        public final ThisT setName(String value)
        {
            if (value != null && value.isEmpty())
            {
                throw new IllegalArgumentException("object name cannot be blank.");
            }
            
            _name = value;
            return getThis();
        }
        
        protected Builder() { }
        
        protected Builder(BuiltT source)
        {
            if (source == null) throw new NullArgumentException("source");
            
            source.setBuilderProperties(getThis());
        }
        
        @SuppressWarnings("unchecked")
        protected ThisT getThis()
        {
            return (ThisT)this;
        }
        
        protected void validate()
        {
            if (_name == null) throw new IllegalStateException("name cannot be null.");
        }
        
        private String _name;
    }

    public final static class ConcreteBuilder extends Builder<ConcreteBuilder, ObjectManifest>
    { }
    
    public final static class ConcreteGsonAdapter extends GsonAdapter<ObjectManifest,
                                                                      ConcreteBuilder>
    {
        @Override
        protected ObjectManifest build(ConcreteBuilder builder)
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
        return other != null
               && getClass() == other.getClass()
               && Objects.equals(_name, ((ObjectManifest)other)._name);
    }

    public ComparisonDataFormat getFormat()
    {
        return ComparisonDataFormat.MINIMAL;
    }
    
    public final String getName()
    {
        return _name;
    }
    
    @Override
    public int hashCode()
    {
        int hash = 0xABadCab5;
        
        hash = hash * 23 ^ _name.hashCode();
        
        return hash;
    }
    
    @Override
    public final String toString()
    {
        return getClass().getName() + "("
               + toStringMembers().map(entry -> entry.getKey() + ": " + entry.getValue())
                                  .collect(Collectors.joining(", "))
               + ")";
    }
    
    // TODO: This should probably be removed now that we're using Gson.
    public final String toJsonString()
    {
        return "{ "
               + toJsonStringMembers().map(entry -> entry.getKey() + ": " + entry.getValue())
                                      .collect(Collectors.joining(", "))
               + " }";
    }
    
    protected static abstract class GsonAdapter<T extends ObjectManifest,
                                                BuilderT extends Builder<?, T>>
            extends AbstractGsonAdapter<T, BuilderT>
    {
        @Override
        protected void readValue(String name, JsonReader in, BuilderT builder) throws IOException
        {
            if (name == null) throw new NullArgumentException("name");
            if (in == null) throw new NullArgumentException("in");
            if (builder == null) throw new NullArgumentException("builder");
            
            if (name == "name")
            {
                builder.setName(in.nextString());
            }
            else
            {
                throw new IOException("Unrecognized attribute: " + javaString(name) + ".");
            }
        }

        @Override
        protected void writeValues(T source, JsonWriter out) throws IOException
        {
            if (source == null) throw new NullArgumentException("source");
            if (out == null) throw new NullArgumentException("out");
            
            out.name("name");
            out.value(source.getName());
        }
    }
    
    protected ObjectManifest(Builder<?, ? extends ObjectManifest> builder)
    {
        if (builder == null) throw new NullArgumentException("builder");
        
        _name = builder.getName();
    }

    protected void setBuilderProperties(Builder<?, ? extends ObjectManifest> builder)
    {
        if (builder == null) throw new NullArgumentException("builder");
        
        builder.setName(getName());
    }
    
    protected Stream<Entry<String, String>> toJsonStringMembers()
    {
        return Stream.of(memberToJsonString("name", javaString(_name)));
    }
    
    protected Stream<Entry<String, String>> toStringMembers()
    {
        return Stream.of(memberToString("name", _name));
    }

    protected static Entry<String, String> memberToJsonString(String name, Object value)
    {
        if (name == null) throw new NullArgumentException("name");
        
        return new SimpleImmutableEntry<>(javaString(name),
                                          value == null ? "null" : value.toString());
    }

    protected static Entry<String, String> memberToString(String name, String value)
    {
        if (name == null) throw new NullArgumentException("name");
        
        return new SimpleImmutableEntry<>(javaString(name), javaString(value));
    }
    
    protected static Entry<String, String> memberToString(String name, Object value)
    {
        if (name == null) throw new NullArgumentException("name");
        
        if (value == null || value instanceof String)
        {
            return memberToString(name, (String)value);
        }
        else
        {
            return new SimpleImmutableEntry<>(javaString(name), value.toString());
        }
    }
    
    private final String _name;
}

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
import com.formationds.iodriver.model.ObjectManifest.GsonAdapter;
import com.google.gson.TypeAdapter;
import com.google.gson.annotations.JsonAdapter;
import com.google.gson.stream.JsonReader;
import com.google.gson.stream.JsonWriter;

@JsonAdapter(GsonAdapter.class)
public class ObjectManifest
{
    public static class Builder<ThisT extends Builder<ThisT>>
    {
        public Builder() { }
        
        public Builder(ObjectManifest source)
        {
            if (source == null) throw new NullArgumentException("source");
            
            source.setBuilderProperties(getThis());
        }
        
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
    
    public static class GsonAdapter extends TypeAdapter<ObjectManifest>
    {
        @Override
        public ObjectManifest read(JsonReader in) throws IOException
        {
            if (in == null) throw new NullArgumentException("in");
            
            Builder<?> builder = new Builder<>();
            readAllValues(in, getAttributeCount(), builder);
            return builder.build();
        }
        
        @Override
        public void write(JsonWriter out, ObjectManifest source) throws IOException
        {
            if (out == null) throw new NullArgumentException("out");
            if (source == null) throw new NullArgumentException("source");
            
            out.beginObject();
            writeValues(source, out);
            out.endObject();
        }

        public static int getAttributeCount()
        {
            return 1;
        }
        
        public static void readValue(String name,
                                     JsonReader in,
                                     Builder<?> builder) throws IOException
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
        
        public static void readAllValues(JsonReader in,
                                         int attributeCount,
                                         Builder<?> builder) throws IOException
        {
            if (in == null) throw new NullArgumentException("in");
            if (builder == null) throw new NullArgumentException("builder");
            
            in.beginObject();
            for (int attribute = 0; attribute != attributeCount; ++attribute)
            {
                readValue(in.nextName(), in, builder);
            }
            in.endObject();
        }
        
        public static void writeValues(ObjectManifest source, JsonWriter out) throws IOException
        {
            if (source == null) throw new NullArgumentException("source");
            if (out == null) throw new NullArgumentException("out");
            
            out.name("name");
            out.value(source._name);
        }
    }
    
    @Override
    public boolean equals(Object other)
    {
        return other != null
               && getClass() == other.getClass()
               && Objects.equals(_name, ((ObjectManifest)other)._name);
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
    
    public final String toJsonString()
    {
        return "{ "
               + toJsonStringMembers().map(entry -> entry.getKey() + ": " + entry.getValue())
                                      .collect(Collectors.joining(", "))
               + " }";
    }
    
    protected ObjectManifest(Builder<?> builder)
    {
        if (builder == null) throw new NullArgumentException("builder");
        
        _name = builder.getName();
    }

    protected void setBuilderProperties(Builder<?> builder)
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

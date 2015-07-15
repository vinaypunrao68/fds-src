package com.formationds.fdsdiff;

import static com.formationds.commons.util.Strings.javaString;

import java.util.AbstractMap.SimpleImmutableEntry;
import java.util.Map.Entry;
import java.util.Objects;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import com.formationds.commons.NullArgumentException;

public class ObjectManifest
{
    public static class Builder<ThisT extends Builder<ThisT>>
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
    
    @Override
    public boolean equals(Object other)
    {
        return other != null
               && getClass() == other.getClass()
               && Objects.equals(_name, ((ObjectManifest)other)._name);
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
            return new SimpleImmutableEntry<>(name, value.toString());
        }
    }
    
    private final String _name;
}

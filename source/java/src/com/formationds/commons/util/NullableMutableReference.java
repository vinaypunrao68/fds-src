package com.formationds.commons.util;

public class NullableMutableReference<T>
{
    public NullableMutableReference()
    {
        this(null);
    }
    
    public NullableMutableReference(T referent)
    {
        _referent = referent;
    }
    
    public T get()
    {
        return _referent;
    }
    
    public void set(T newReferent)
    {
        _referent = newReferent;
    }
    
    private T _referent;
}

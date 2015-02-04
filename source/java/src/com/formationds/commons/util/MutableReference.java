package com.formationds.commons.util;

import com.formationds.commons.NullArgumentException;

public class MutableReference<T> extends NullableMutableReference<T>
{
    public MutableReference(T referent)
    {
        super(notNull(referent, "referent"));
    }
    
    @Override
    public T get()
    {
        return super.get();
    }
    
    @Override
    public void set(T newReferent)
    {
        if (newReferent == null) throw new NullArgumentException("newReferent");
        
        super.set(newReferent);
    }
    
    private static <T> T notNull(T value, String name)
    {
        if (name == null) throw new NullArgumentException("name");
        
        if (value == null)
        {
            throw new NullArgumentException(name);
        }
        
        return value;
    }
}

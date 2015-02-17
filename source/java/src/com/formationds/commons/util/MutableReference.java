package com.formationds.commons.util;

import com.formationds.commons.NullArgumentException;

/**
 * A reference to another object that may be changed.
 * 
 * @param <T> The type of referent.
 */
public class MutableReference<T> extends NullableMutableReference<T>
{
    /**
     * Constructor.
     * 
     * @param referent The object to point to.
     */
    public MutableReference(T referent)
    {
        super(notNull(referent, "referent"));
    }
    
    /**
     * Get the pointed-to object.
     */
    @Override
    public T get()
    {
        return super.get();
    }
    
    /**
     * Point to a new object.
     */
    @Override
    public void set(T newReferent)
    {
        if (newReferent == null) throw new NullArgumentException("newReferent");
        
        super.set(newReferent);
    }
    
    /**
     * Verify that a referent is not null.
     * 
     * @param value The value to check.
     * @param name The name of the argument for errors.
     * 
     * @return {@code value}.
     */
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

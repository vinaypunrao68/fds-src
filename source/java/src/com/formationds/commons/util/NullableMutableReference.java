package com.formationds.commons.util;

/**
 * A reference to another object that may be {@code null}.
 * 
 * @param <T> The type of the referent.
 */
public class NullableMutableReference<T>
{
    /**
     * Constructor. Referent will be {@code null}.
     */
    public NullableMutableReference()
    {
        this(null);
    }
    
    /**
     * Constructor.
     * 
     * @param referent The object to point to.
     */
    public NullableMutableReference(T referent)
    {
        _referent = referent;
    }
    
    /**
     * Get the pointed-to object.
     * 
     * @return The object, or {@code null}.
     */
    public T get()
    {
        return _referent;
    }
    
    /**
     * Point to a new object.
     * 
     * @param newReferent The object to point to, or {@code null}.
     */
    public void set(T newReferent)
    {
        _referent = newReferent;
    }
    
    /**
     * The object pointed to, or {@code null}.
     */
    private T _referent;
}

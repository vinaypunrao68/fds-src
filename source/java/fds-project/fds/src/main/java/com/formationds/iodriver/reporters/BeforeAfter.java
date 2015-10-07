package com.formationds.iodriver.reporters;

public final class BeforeAfter<T>
{
    public BeforeAfter(T before, T after)
    {
        _before = before;
        _after = after;
    }
    
    public T getAfter()
    {
        return _after;
    }
    
    public T getBefore()
    {
        return _before;
    }
    
    private final T _after;
    
    private final T _before;
}
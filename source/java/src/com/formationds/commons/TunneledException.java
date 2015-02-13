package com.formationds.commons;

public class TunneledException extends RuntimeException
{
    public TunneledException(Throwable innerException)
    {
        super(null, verifyNotNull(innerException));
    }
    
    private static final long serialVersionUID = 1L;
    
    private static Throwable verifyNotNull(Throwable innerException)
    {
        if (innerException == null) throw new NullArgumentException("innerException");
        
        return innerException;
    }
}

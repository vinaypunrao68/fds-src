package com.formationds.iodriver;

public class TunneledException extends RuntimeException
{
    public TunneledException(Throwable innerException)
    {
        super(null, innerException);
    }
    
    private static final long serialVersionUID = 1L;
}

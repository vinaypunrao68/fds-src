package com.formationds.commons;

/**
 * Wraps a checked exception that is not declared in the function signature it's passing through.
 * This class should generally not be used except through
 * {@link com.formationds.commons.util.ExceptionHelper#tunnel() ExceptionHelper#tunnel()} and
 * friends.
 */
public class TunneledException extends RuntimeException
{
    /**
     * Constructor.
     * 
     * @param innerException The exception to tunnel.
     */
    public TunneledException(Throwable innerException)
    {
        super(null, verifyNotNull(innerException));
    }

    /**
     * Serialization version.
     */
    private static final long serialVersionUID = 1L;

    /**
     * Ensure the passed value is not null.
     * 
     * @param innerException The exception to tunnel.
     * 
     * @return {@code innerException}.
     */
    private static Throwable verifyNotNull(Throwable innerException)
    {
        if (innerException == null) throw new NullArgumentException("innerException");

        return innerException;
    }
}

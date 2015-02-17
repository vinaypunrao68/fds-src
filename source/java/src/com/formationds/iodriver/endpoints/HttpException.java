package com.formationds.iodriver.endpoints;

/**
 * An error occurred while processing an HTTP connection.
 */
public class HttpException extends Exception
{
    /**
     * Constructor.
     * 
     * @param message Detailed user message.
     * @param cause Proximate cause of this error.
     */
    public HttpException(String message, Throwable cause)
    {
        super(message, cause);
    }

    /**
     * Constructor.
     * 
     * @param responseCode The HTTP response code.
     */
    public HttpException(int responseCode)
    {
        this(responseCode, null);
    }

    /**
     * Constructor.
     * 
     * @param responseCode The HTTP response code.
     * @param responseMessage The HTTP response message.
     */
    public HttpException(int responseCode, String responseMessage)
    {
        this(responseCode, responseMessage, null, null);
    }

    /**
     * Constructor.
     * 
     * @param responseCode The HTTP response code.
     * @param responseMessage The HTTP response message.
     * @param errorResponse The text of the response.
     * @param cause The proximate cause of this error.
     */
    public HttpException(int responseCode,
                         String responseMessage,
                         String errorResponse,
                         Throwable cause)
    {
        this("HTTP response " + responseCode + ": " + responseMessage + "\n\n" + errorResponse,
             cause);
    }

    private static final long serialVersionUID = 1L;
}

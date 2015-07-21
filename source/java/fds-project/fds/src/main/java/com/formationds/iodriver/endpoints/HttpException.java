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
     * @param resource The URI that was requested.
     * @param method The HTTP method used.
     */
    public HttpException(int responseCode, String resource, String method)
    {
        this(responseCode, resource, method, null);
    }

    /**
     * Constructor.
     * 
     * @param responseCode The HTTP response code.
     * @param resource The URI that was requested.
     * @param method The HTTP method used.
     * @param responseMessage The HTTP response message.
     */
    public HttpException(int responseCode, String resource, String method, String responseMessage)
    {
        this(responseCode, resource, method, responseMessage, null, null);
    }

    /**
     * Constructor.
     * 
     * @param responseCode The HTTP response code.
     * @param resource The URI that was requested.
     * @param method The HTTP method used.
     * @param responseMessage The HTTP response message.
     * @param errorResponse The text of the response.
     * @param cause The proximate cause of this error.
     */
    public HttpException(int responseCode,
                         String resource,
                         String method,
                         String responseMessage,
                         String errorResponse,
                         Throwable cause)
    {
        this("HTTP response " + responseCode + " attempting " + method + " from URI: " + resource
             + " -- " + responseMessage + "\n\n" + errorResponse,
             cause);
    }

    private static final long serialVersionUID = 1L;
}

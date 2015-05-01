package com.formationds.iodriver.endpoints;

/**
 * An error occurred on the HTTP server's side.
 */
public class HttpServerException extends HttpException
{
    /**
     * Constructor.
     * 
     * @param responseCode The HTTP response code.
     * @param responseMessage The HTTP response message.
     * @param errorResponse The text of the response.
     */
    public HttpServerException(int responseCode, String responseMessage, String errorResponse)
    {
        this(responseCode, responseMessage, errorResponse, null);
    }

    /**
     * Constructor.
     * 
     * @param responseCode The HTTP response code.
     * @param responseMessage The HTTP response message.
     * @param errorResponse The text of the response.
     * @param cause The proximate cause of this error.
     */
    public HttpServerException(int responseCode,
                               String responseMessage,
                               String errorResponse,
                               Throwable cause)
    {
        super(responseCode, responseMessage, errorResponse, cause);
    }

    /**
     * Serialization format.
     */
    private static final long serialVersionUID = 1L;
}

package com.formationds.iodriver.endpoints;

/**
 * An error occurred on the HTTP client's side.
 */
public class HttpClientException extends HttpException
{
    /**
     * Constructor.
     * 
     * @param responseCode The HTTP response code.
     * @param responseMessage The HTTP response message.
     * @param errorResponse The text of the response.
     */
    public HttpClientException(int responseCode, String responseMessage, String errorResponse)
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
    public HttpClientException(int responseCode,
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

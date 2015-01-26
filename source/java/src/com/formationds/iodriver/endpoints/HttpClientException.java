package com.formationds.iodriver.endpoints;

public class HttpClientException extends HttpException
{
    public HttpClientException(int responseCode, String responseMessage, String errorResponse)
    {
        this(responseCode, responseMessage, errorResponse, null);
    }

    public HttpClientException(int responseCode,
                               String responseMessage,
                               String errorResponse,
                               Throwable cause)
    {
        super(responseCode, responseMessage, errorResponse, cause);
    }

    private static final long serialVersionUID = 1L;
}

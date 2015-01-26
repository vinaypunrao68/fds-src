package com.formationds.iodriver.endpoints;

public class HttpException extends Exception
{
    public HttpException(String message, Throwable cause)
    {
        super(message, cause);
    }

    public HttpException(int responseCode)
    {
        this(responseCode, null);
    }

    public HttpException(int responseCode, String responseMessage)
    {
        this(responseCode, responseMessage, null, null);
    }

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

package com.formationds.sc;

public class SvcException extends Throwable {
    private final FdsError error;

    public SvcException(int errorCode) {
        error = FdsError.findByErrorCode(errorCode);
    }

    @Override
    public String getMessage() {
        return error.getMessage();
    }

    public FdsError getFdsError() {
        return error;
    }
}

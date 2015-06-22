package com.formationds.util.s3.auth;

public class ChunkDecodingException extends Exception {
    public ChunkDecodingException(String message) {
        super(message);
    }

    public ChunkDecodingException(String message, Throwable cause) {
        super(message, cause);
    }
}

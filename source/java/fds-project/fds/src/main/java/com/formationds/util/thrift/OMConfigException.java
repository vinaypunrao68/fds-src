/*
 * Copyright (c) 2015 Formation Data Systems. All rights Reserved.
 */

package com.formationds.util.thrift;

/**
 * Generic OM Configuration Client exception
 */
public class OMConfigException extends Exception {
    public OMConfigException(String message, Object... messageArgs) {
        super(String.format(message, messageArgs));
    }

    public OMConfigException(Throwable cause, String message, String... messageArgs) {
        super(String.format(message, messageArgs), cause);
    }
}

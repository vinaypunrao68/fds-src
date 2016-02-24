/*
 * Copyright (c) 2015 Formation Data Systems. All rights Reserved.
 */

package com.formationds.util.thrift;

import com.formationds.protocol.ApiException;
import com.formationds.protocol.ErrorCode;

/**
 * Generic OM Configuration Client exception
 */
public class OMConfigException extends ApiException {

    public OMConfigException( ErrorCode errorCode, String message ) {
        super( message, errorCode );
    }

    public OMConfigException(ErrorCode ec, String message, Object... messageArgs) {
        super(String.format(message, messageArgs), ec);
    }
}

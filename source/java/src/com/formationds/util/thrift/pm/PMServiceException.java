/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.util.thrift.pm;


/**
 * @author ptinius
 */
public class PMServiceException
    extends Exception {

    private static final long serialVersionUID = 1745422230467776180L;

    public PMServiceException( final String message,
                               final Object[] messageArgs) {

        super( String.format( message, messageArgs ) );
    }

    public PMServiceException( final Throwable cause,
                               final String message,
                               final Object[] messageArgs ) {

        super( String.format( message, messageArgs ), cause );
    }
}

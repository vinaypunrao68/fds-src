/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.util.thrift.svc;


/**
 * @author ptinius
 */
public class SvcLayerException
    extends Exception {

    private static final long serialVersionUID = 1745422230467776180L;

    public SvcLayerException( final String message,
                              final Object[] messageArgs ) {

        super( String.format( message, messageArgs ) );
    }

    public SvcLayerException( final Throwable cause,
                              final String message,
                              final Object[] messageArgs ) {

        super( String.format( message, messageArgs ), cause );
    }
}

/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.util;

import javax.xml.bind.DatatypeConverter;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.ObjectOutputStream;
import java.io.Serializable;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

/**
 * @author ptinius
 */
public class ObjectUtils
{
    private static final String ALGORITHM = "MD5";

    /**
     * @param object the {@link Serializable} {@link Object} to checksum
     *
     * @return Returns {@link String} representing the MD5 hex checksum
     *
     * @throws IOException
     * @throws NoSuchAlgorithmException
     */
    public static String checksum( final Serializable object )
        throws IOException, NoSuchAlgorithmException
    {
        try( ByteArrayOutputStream baos = new ByteArrayOutputStream( );
             ObjectOutputStream oos = new ObjectOutputStream( baos ) )
        {
            oos.writeObject( object );
            return DatatypeConverter.printHexBinary(
                MessageDigest.getInstance( ALGORITHM )
                             .digest( baos.toByteArray( ) ) );
        }
    }
}

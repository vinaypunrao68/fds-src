/*
 * Copyright (c) 2016, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.converters;

import com.formationds.client.v08.model.nfs.NfsClients;
import com.formationds.client.v08.model.nfs.NfsOptions;
import com.formationds.protocol.NfsOption;
import org.junit.Before;
import org.junit.Test;

/**
 * @author ptinius
 */
public class ExternalModelConverterTest
{
    final NfsOption internal = new NfsOption( );

    @Before
    public void setUp( )
        throws Exception
    {
        internal.setClient( "*" );
        /*
         * OPTION: 'acl'
         * OPTION: 'no_root_squash'
         * OPTION: 'async'
         */
//        internal.setOptions( "noacl,no_root_squash,async" );
        internal.setOptions( "" );
    }

    @Test
    public void testConvertToExternalNfsOptions( )
        throws Exception
    {
        final NfsOptions externalOptions =
            ExternalModelConverter.convertToExternalNfsOptions( internal );
        System.out.println( externalOptions.getOptions() );
    }

    @Test
    public void testConvertToExternalNfsClients( )
        throws Exception
    {
        final NfsClients externalClients =
            ExternalModelConverter.convertToExternalNfsClients( internal );
        System.out.println( externalClients.getClient() );
    }
}

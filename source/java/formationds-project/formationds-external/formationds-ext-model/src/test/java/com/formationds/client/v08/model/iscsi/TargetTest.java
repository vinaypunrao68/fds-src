/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model.iscsi;

import org.junit.Assert;
import org.junit.Test;

import java.util.UUID;

/**
 * @author ptinius
 */
public class TargetTest
{
    @Test
    public void testTargetGroup() {
        final Target target =
            new Target.Builder()
                      .withIncomingUser( new Credentials( "username",
                                                           "userpassword" ) )
                      .withIncomingUser( new Credentials( "username1",
                                                           "userpassword1" ) )
                      .withOutgoingUser( new Credentials( "ouser","opasswd" ) )
                      .withLun( new LUN.Builder()
                                       .withLun( "volume_0" )
                                       .withAccessType( LUN.AccessType.RW )
                                       .withInitiator( new Initiator( "82:*:00:*" ) )
                                       .withInitiator( new Initiator( "83:*:00:*" ) )
                                       .build( ) )
                      .build( );

        target.setId( 0L );
        target.setName( UUID.randomUUID().toString() );

        Assert.assertEquals( target.getId( )
                                   .longValue( ), 0L );
        Assert.assertNotNull( target.getName( ) );

        Assert.assertTrue( target.getIncomingUsers().size() == 2 );
        for( final Credentials user : target.getIncomingUsers() )
        {
            Assert.assertNotNull( user.getName() );
            Assert.assertNotNull( user.getPasswd( ) );
        }

        Assert.assertTrue( target.getOutgoingUsers().size() == 1 );
        for( final Credentials user : target.getOutgoingUsers( ) )
        {
            Assert.assertNotNull( user.getName() );
            Assert.assertNotNull( user.getPasswd() );
        }

        Assert.assertTrue( target.getLuns().get( 0 ).getInitiators().size() == 2 );
        for( final Initiator initiator : target.getLuns().get( 0 ).getInitiators() )
        {
            Assert.assertNotNull( initiator.getWWNMask() );
        }

        Assert.assertTrue( target.getLuns( ).size( ) == 1 );
        for( final LUN lun : target.getLuns() )
        {
            Assert.assertNotNull( lun.getLunName() );
            Assert.assertNotNull( lun.getAccessType() );
        }
    }
}

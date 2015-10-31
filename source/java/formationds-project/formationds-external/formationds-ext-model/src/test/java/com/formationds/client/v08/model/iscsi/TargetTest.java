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
                      .withIncomingUser( new IncomingUser( "username",
                                                           "userpassword" ) )
                      .withIncomingUser( new IncomingUser( "username1",
                                                           "userpassword1" ) )
                      .withOutgoingUser( new OutgoingUser( "ouser","opasswd" ) )
                      .withLun( new LUN.Builder()
                                       .withLun( "volume_0" )
                                       .withAccessType( LUN.AccessType.RW )
                                       .build( ) )
                      .withInitiator( new Initiator( "82:*:00:*" ) )
                      .withInitiator( new Initiator( "83:*:00:*" ) )
                      .build( );

        target.setId( 0L );
        target.setName( UUID.randomUUID().toString() );

        Assert.assertEquals( target.getId( )
                                   .longValue( ), 0L );
        Assert.assertNotNull( target.getName( ) );

        Assert.assertTrue( target.getIncomingUsers().size() == 2 );
        for( final IncomingUser user : target.getIncomingUsers() )
        {
            Assert.assertNotNull( user.getName() );
            Assert.assertNotNull( user.getPasswd( ) );
        }

        Assert.assertTrue( target.getOutgoingUsers().size() == 1 );
        for( final OutgoingUser user : target.getOutgoingUsers( ) )
        {
            Assert.assertNotNull( user.getName() );
            Assert.assertNotNull( user.getPasswd() );
        }

        Assert.assertTrue( target.getInitiators().size() == 2 );
        for( final Initiator initiator : target.getInitiators() )
        {
            Assert.assertNotNull( initiator.getWWN() );
        }

        Assert.assertTrue( target.getLuns( ).size( ) == 1 );
        for( final LUN lun : target.getLuns() )
        {
            Assert.assertNotNull( lun.getLunName() );
            Assert.assertNotNull( lun.getAccessType() );
        }
    }
}

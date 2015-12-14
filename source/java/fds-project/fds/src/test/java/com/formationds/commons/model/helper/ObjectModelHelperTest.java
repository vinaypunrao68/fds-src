/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.helper;

import com.formationds.client.v08.model.iscsi.Credentials;
import com.formationds.client.v08.model.iscsi.Initiator;
import com.formationds.client.v08.model.iscsi.LUN;
import com.formationds.client.v08.model.iscsi.Target;
import org.junit.Test;

/**
 * @author ptinius
 */
public class ObjectModelHelperTest {

//  @Test
//  public void test() {
//    final List<Feature> features = new ArrayList<>();
//    features.add( Feature.SYS_MGMT );
//    features.add( Feature.USER_MGMT );
//
//    final AuthenticatedUser user =
//      new AuthenticatedUserBuilder().withFeatures( features )
//                                    .withToken( "1209809FABCD" )
//                                    .withUserId( 11234L )
//                                    .withUsername( "Three Little Pigs" )
//                                    .build();
//
//    final AuthenticatedUser user1 =
//      ObjectModelHelper.toObject( ObjectModelHelper.toJSON( user ),
//                                  AuthenticatedUser.class );
//
//    Assert.assertEquals( user.getUserId(), user1.getUserId() );
//    Assert.assertEquals( user.getUsername(), user1.getUsername() );
//    Assert.assertEquals( user.getToken(), user1.getToken() );
//    Assert.assertEquals( user.getFeatures(), user1.getFeatures() );
//  }

//  @Test
//  public void jsonTest()
//  {
//    try {
//      final SnapshotPolicy expected =
//        new SnapshotPolicyBuilder().withId( 2 )
//                                   .withName( "snapshot policy name #2" )
//                                   .withRecurrenceRule(
//                                     new RecurrenceRule( "RRULE:FREQ=MONTHLY;BYDAY=SA;BYMONTHDAY=7,8,9,10,11,12,13" ) )
//                                   .withRetention( 34567 )
//                                   .build();
//
//      final SnapshotPolicy actual =
//        ObjectModelHelper.toObject( ObjectModelHelper.toJSON( expected ),
//                                                              SnapshotPolicy.class );
//
//      Assert.assertEquals( expected.getId(), actual.getId() );
//      Assert.assertEquals( expected.getName(), actual.getName() );
//      Assert.assertEquals( expected.getRecurrenceRule().getRecurrenceRule(),
//                           actual.getRecurrenceRule().getRecurrenceRule() );
//      Assert.assertEquals( expected.getRetention(), actual.getRetention() );
//    } catch( ParseException e ) {
//      Assert.fail( e.getMessage() );
//      e.printStackTrace();
//    }
//  }

//  @Test
//  public void dateTest() {
//    final List<String> dates =
//      Arrays.asList( "19970902T170000Z", "20000131T090000Z" );
//
//    for( final String date : dates ) {
//      try {
//        System.out.println( ObjectModelHelper.toiCalFormat( date ) );
//      } catch( ParseException e ) {
//        e.printStackTrace();
//      }
//    }
//  }

    @Test
    public void createTarget()
    {
        final Target target =
            new Target.Builder()
                .withIncomingUser( new Credentials( "username",
                                                    "userpassword" ) )
                .withIncomingUser( new Credentials( "username1",
                                                    "userpassword1" ) )
                .withOutgoingUser( new Credentials( "ouser", "opasswd" ) )
                .withLun( new LUN.Builder()
                              .withLun( "volume_0" )
                              .withAccessType( LUN.AccessType.RW )
                              .build( ) )
                .withInitiator( new Initiator( "82:*:00:*" ) )
                .withInitiator( new Initiator( "83:*:00:*" ) )
                .build( );

        System.out.println( ObjectModelHelper.toJSON( target ) );
    }

}

/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model;

import com.formationds.client.ical.RecurrenceRule;
import com.formationds.client.ical.WeekDays;
import com.formationds.client.ical.iCalWeekDays;
import com.formationds.client.v08.model.iscsi.Credentials;
import com.formationds.client.v08.model.iscsi.Initiator;
import com.formationds.client.v08.model.iscsi.LUN;
import com.formationds.client.v08.model.iscsi.Target;
import com.formationds.commons.model.helper.ObjectModelHelper;
import org.junit.Test;

import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.List;

/**
 * @author ptinius
 */
public class ISCSITest
{
    @Test
    public void test()
    {
        final Target target =
            new Target.Builder().withLun( new LUN.Builder( )
                                                 .withLun( "volume1" )
                                                 .withAccessType( LUN.AccessType.RW )
                                                 .build() )
                                .withIncomingUser( new Credentials( "Iuser1", "Ipasswd1" ) )
                                .withOutgoingUser( new Credentials( "Ouser1", "Opasswd1" ) )
                                .withInitiator( new Initiator( "iqn.2010-07.com.example:node1" ) )
                                .build( );

        final VolumeSettingsISCSI settings =
            new VolumeSettingsISCSI( Size.gb( 10 ), Size.kb( 128 ), target );

        RecurrenceRule rule = new RecurrenceRule();
        rule.setFrequency( "WEEKLY" );

        WeekDays days = new WeekDays();
        days.add( iCalWeekDays.MO );

        rule.setDays( days );

        SnapshotPolicy sPolicy = new SnapshotPolicy( SnapshotPolicy.SnapshotPolicyType.SYSTEM_TIMELINE, rule, Duration.ofDays( 30 ) );
        List<SnapshotPolicy> sPolicies = new ArrayList<>();
        sPolicies.add( sPolicy );

        QosPolicy qPolicy = new QosPolicy( 3, 0, 2000 );

        DataProtectionPolicy dPolicy = new DataProtectionPolicy( Duration.ofDays( 1L ), sPolicies );

        Tenant tenant = new Tenant( 3L, "Pepsi" );
        VolumeStatus status = new VolumeStatus( VolumeState.Active,
                                                Size.of( 3, SizeUnit.GB ) );

        final Volume volume = new Volume( 1L,
                                          "TestVolume",
                                          tenant,
                                          "MarioBrothers",
                                          status,
                                          settings,
                                          MediaPolicy.HDD,
                                          dPolicy,
                                          VolumeAccessPolicy.exclusiveRWPolicy(),
                                          qPolicy,
                                          Instant.now( ),
                                          null );

        System.out.println( ObjectModelHelper.toJSON( volume ) );
    }

    @Test
    public void test_1()
    {
        final String x = "{\"capacity\": {\"value\": 10, \"unit\": \"GB\"}, \"type\": \"ISCSI\", \"target\": {\"luns\": [{\"lunName\": \"1\", \"accessType\": \"RO\"}], \"incomingUsers\": [{\"username\": \"Bob\", \"password\": \"Test\"}, {\"username\": \"Phyllis\", \"password\": \"Toast\"}], \"initiators\": [{\"wwn_mask\": \"me\"}, {\"wwn_mask\": \"you\"}, {\"wwn_mask\": \"them.*\"}, {\"wwn_mask\": \"him\"}, {\"wwn_mask\": \"her:\"}], \"outgoingUsers\": [{\"username\": \"Mack\", \"password\": \"Fires\"}, {\"username\": \"Mort\", \"password\": \"trom\"}]}}";
        final VolumeSettings volume = ObjectModelHelper.toObject( x, VolumeSettings.class );
        System.out.println( ObjectModelHelper.toJSON( volume ) );
    }
}

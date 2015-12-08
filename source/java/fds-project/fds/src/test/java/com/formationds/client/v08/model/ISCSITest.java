/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model;

import com.formationds.client.ical.RecurrenceRule;
import com.formationds.client.ical.WeekDays;
import com.formationds.client.ical.iCalWeekDays;
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
}

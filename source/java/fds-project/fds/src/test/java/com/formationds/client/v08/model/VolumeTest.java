package com.formationds.client.v08.model;

import com.formationds.client.ical.RecurrenceRule;
import com.formationds.client.ical.WeekDays;
import com.formationds.client.ical.iCalWeekDays;
import com.formationds.client.v08.model.SnapshotPolicy.SnapshotPolicyType;
import com.formationds.client.v08.model.iscsi.LUN;
import com.formationds.client.v08.model.iscsi.Target;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.webkit.rest.v08.presets.GetDataProtectionPolicyPresets;
import org.junit.Test;

import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.List;

public class VolumeTest {

    @Test
    public void testBasicVolumeMarshalling() {

        Tenant tenant = new Tenant( 3L, "Dave" );
        VolumeStatus status = new VolumeStatus( VolumeState.Active,
                                                Size.of( 3, SizeUnit.GB ) );

//        VolumeSettings settings = new VolumeSettingsBlock( Size.of( 5L, SizeUnit.TERABYTE ),
//                                                           Size.of( 1, SizeUnit.KILOBYTE ) );
        VolumeSettings settings = new VolumeSettingsObject( Size.of( 1L, SizeUnit.KB ) );
        
        RecurrenceRule rule = new RecurrenceRule();
        rule.setFrequency( "WEEKLY" );

        WeekDays days = new WeekDays();
        days.add( iCalWeekDays.MO );

                  rule.setDays( days );

        SnapshotPolicy sPolicy = new SnapshotPolicy( SnapshotPolicyType.SYSTEM_TIMELINE, rule, Duration.ofDays( 30 ) );
        List<SnapshotPolicy> sPolicies = new ArrayList<>();
        sPolicies.add( sPolicy );

        QosPolicy qPolicy = new QosPolicy( 3, 0, 2000 );

        DataProtectionPolicy dPolicy = new DataProtectionPolicy( Duration.ofDays( 1L ), sPolicies );

        Volume basicVolume = new Volume( 1L,
                                         "TestVolume",
                                         tenant,
                                         "MarioBrothers",
                                         status,
                                         settings,
                                         MediaPolicy.HDD,
                                         dPolicy,
                                         VolumeAccessPolicy.exclusiveRWPolicy(),
                                         qPolicy,
                                         Instant.now(),
                                         null );

        String jsonString = ObjectModelHelper.toJSON( basicVolume );
        Volume readVolume = ObjectModelHelper.toObject( jsonString, Volume.class );

        assert readVolume.getId().equals( basicVolume.getId() );
        assert readVolume.getName().equals( basicVolume.getName() );
    }

    @Test
    public void testVolumeIscsiJson()
    {
        final Target target = new Target.Builder()
                                        .withLun( new LUN.Builder()
                                                         .withLun( "testVolume_iscsi" )
                                                         .withAccessType( LUN.AccessType.RW )
                                                         .build() )
                                        .build();
        final Volume volume = new Volume.Builder( "TestVolume_1" )
                                        .application( "Candy Crush" )
                                        .settings( new VolumeSettingsISCSI.Builder()
                                                                  .withTarget( target )
                                                                  .withBlockSize( Size.kb( 128L ) )
                                                                  .withCapacity( Size.gb( 10L ) )
                                                                  .build() )
                                        .qosPolicy( QosPolicy.of( 7, 10000, 100000 ) )
                                        .mediaPolicy( MediaPolicy.HYBRID )
                                        .accessPolicy( VolumeAccessPolicy.nonExclusivePolicy() )
                                        .dataProtectionPolicy( new GetDataProtectionPolicyPresets().getDataProtectionPolicyPresets().get( 0 ) )
                                        .create();

        System.out.println( ObjectModelHelper.toJSON( volume ) );
    }
}

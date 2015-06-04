package com.formationds.client.v08.model;

import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.List;

import org.junit.Test;

import com.formationds.client.ical.RecurrenceRule;
import com.formationds.client.ical.WeekDays;
import com.formationds.client.v08.model.DataProtectionPolicy;
import com.formationds.client.v08.model.MediaPolicy;
import com.formationds.client.v08.model.QosPolicy;
import com.formationds.client.v08.model.Size;
import com.formationds.client.v08.model.SizeUnit;
import com.formationds.client.v08.model.SnapshotPolicy;
import com.formationds.client.v08.model.Tenant;
import com.formationds.client.v08.model.Volume;
import com.formationds.client.v08.model.VolumeAccessPolicy;
import com.formationds.client.v08.model.VolumeSettings;
import com.formationds.client.v08.model.VolumeSettings.BlockVolumeSettings;
import com.formationds.client.v08.model.VolumeState;
import com.formationds.client.v08.model.VolumeStatus;
import com.formationds.commons.model.helper.ObjectModelHelper;

public class VolumeTest {

	@Test
	public void testBasicVolumeMarshalling(){
		
		Tenant tenant = new Tenant( 3L, "Dave" );
		
		VolumeStatus status = new VolumeStatus( VolumeState.Active,
											    new Size<Integer>( 3, SizeUnit.GIGABYTE) ); 
		
		VolumeSettings settings = new BlockVolumeSettings( new Size<Long>( 5L, SizeUnit.TERABYTE ),
				                                           new Size<Integer>( 1, SizeUnit.KILOBYTE ) );
		
		RecurrenceRule rule = new RecurrenceRule();
		rule.setFrequency( "WEEKLY" );
		
		WeekDays<String> days = new WeekDays<String>();
		days.add( "MO" );
		
		rule.setDays(days);
		
		SnapshotPolicy sPolicy = new SnapshotPolicy( rule, Duration.ofDays( 30 ) );
		List<SnapshotPolicy> sPolicies = new ArrayList<>();
		sPolicies.add( sPolicy );
		
		QosPolicy qPolicy = new QosPolicy( 3, 0, 2000 );
		
		DataProtectionPolicy dPolicy = new DataProtectionPolicy( Duration.ofDays(1L), sPolicies );
		
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
}

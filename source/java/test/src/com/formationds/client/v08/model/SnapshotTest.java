package com.formationds.client.v08.model;

import java.time.Duration;
import java.time.Instant;

import org.junit.Test;

import com.formationds.commons.model.helper.ObjectModelHelper;

public class SnapshotTest {

	@Test
	public void testSnapshotConversion(){
		
		Snapshot snapshot = new Snapshot( 8L, "8", 3L, Duration.ofDays( 1L ), Instant.now() );
		
		String jsonString = ObjectModelHelper.toJSON( snapshot );
		
		Snapshot newSnapshot = ObjectModelHelper.toObject( jsonString, Snapshot.class );
		
		assert newSnapshot.getId().equals( snapshot.getId() );
		assert newSnapshot.getName().equals( snapshot.getName() );
		assert newSnapshot.getVolumeId().equals( snapshot.getVolumeId() );
		assert newSnapshot.getRetention().equals( snapshot.getRetention() );
		assert newSnapshot.getCreationTime().equals( snapshot.getCreationTime() );
	}
	
}

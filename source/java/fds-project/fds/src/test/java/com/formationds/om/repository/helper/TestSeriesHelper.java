package com.formationds.om.repository.helper;

import com.formationds.commons.model.Datapoint;
import com.formationds.commons.model.Series;
import com.formationds.commons.model.entity.IVolumeDatapoint;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.type.Metrics;
import com.formationds.commons.model.type.StatOperation;
import org.junit.BeforeClass;
import org.junit.Test;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;

import static org.junit.Assert.assertEquals;

public class TestSeriesHelper {

	private static List<IVolumeDatapoint> pbyteResults;
	private static List<IVolumeDatapoint> ubyteResults;

	@BeforeClass
	public static void initialize(){
		pbyteResults = new ArrayList<>();

		pbyteResults.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 1L ), "1", "TestVol", "PBYTES", 1.0 ) );
		pbyteResults.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 2L ), "1", "TestVol", "PBYTES", 1.2 ) );
		pbyteResults.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 30L ), "1", "TestVol", "PBYTES", 100004.1 ) );
		pbyteResults.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 15L ), "1", "TestVol", "PBYTES", 10186.33 ) );
		pbyteResults.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 12L ), "1", "TestVol", "PBYTES", 4006.0 ) );
		pbyteResults.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 110L ), "1", "TestVol", "PBYTES", 1000000.0 ) );
		pbyteResults.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 7L ), "1", "TestVol", "PBYTES", 10.1 ) );
		pbyteResults.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 8L ), "1", "TestVol", "PBYTES", 18.64 ) );
		pbyteResults.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 11L ), "1", "TestVol", "PBYTES", 33.0 ) );
		pbyteResults.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 3L ), "1", "TestVol", "PBYTES", 1.9 ) );
		pbyteResults.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 4L ), "1", "TestVol", "PBYTES", 3.0 ) );
		pbyteResults.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 5L ), "1", "TestVol", "PBYTES", 4.1 ) );
		pbyteResults.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 6L ), "1", "TestVol", "PBYTES", 5.9 ) );

		ubyteResults = new ArrayList<>();

		ubyteResults.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 1L ), "1", "TestVol", "UBYTES", 1.0 ) );
		ubyteResults.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 2L ), "1", "TestVol", "UBYTES", 1.2 ) );
		ubyteResults.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 30L ), "1", "TestVol", "UBYTES", 100004.1 ) );
		ubyteResults.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 15L ), "1", "TestVol", "UBYTES", 10186.33 ) );
		ubyteResults.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 12L ), "1", "TestVol", "UBYTES", 4006.0 ) );
		ubyteResults.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 110L ), "1", "TestVol", "UBYTES", 1000000.0 ) );
		ubyteResults.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 7L ), "1", "TestVol", "UBYTES", 10.1 ) );
		ubyteResults.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 8L ), "1", "TestVol", "UBYTES", 18.64 ) );
		ubyteResults.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 11L ), "1", "TestVol", "UBYTES", 33.0 ) );
		ubyteResults.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 3L ), "1", "TestVol", "UBYTES", 1.9 ) );
		ubyteResults.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 4L ), "1", "TestVol", "UBYTES", 3.0 ) );
		ubyteResults.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 5L ), "1", "TestVol", "UBYTES", 4.1 ) );
		ubyteResults.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 6L ), "1", "TestVol", "UBYTES", 5.9 ) );
	}
	
	@Test
	public void testPbytesMax() {

		SeriesHelper sh = new SeriesHelper();
		
		Series s = sh.generate( pbyteResults, 0L, Metrics.PBYTES, 10L, 100, StatOperation.MAX_X );
		
		assertEquals( 4, s.getDatapoints().size() );
		
		Double maxSize = s.getDatapoints().stream().mapToDouble( Datapoint::getY ).max().getAsDouble();
		assertEquals( new Double(1000000.0), maxSize);
		
		assertEquals( new Double( 1000000.0 ), s.getDatapoints().get( 3 ).getY() );
	}

	@Test
	public void testUbytesMax() {

		SeriesHelper sh = new SeriesHelper();

		Series s = sh.generate( ubyteResults, 0L, Metrics.UBYTES, 10L, 100, StatOperation.MAX_X );

		assertEquals( 4, s.getDatapoints().size() );

		Double maxSize = s.getDatapoints().stream().mapToDouble( Datapoint::getY ).max().getAsDouble();
		assertEquals( new Double(1000000.0), maxSize);

		assertEquals( new Double( 1000000.0 ), s.getDatapoints().get( 3 ).getY() );
	}
}

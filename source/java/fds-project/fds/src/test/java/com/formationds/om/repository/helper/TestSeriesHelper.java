package com.formationds.om.repository.helper;

import static org.junit.Assert.*;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;

import org.junit.BeforeClass;
import org.junit.Test;

import com.formationds.commons.model.Datapoint;
import com.formationds.commons.model.Series;
import com.formationds.commons.model.entity.IVolumeDatapoint;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.type.Metrics;
import com.formationds.commons.model.type.StatOperation;

public class TestSeriesHelper {

	private static List<IVolumeDatapoint> results;

	@BeforeClass
	public static void initialize(){
		results = new ArrayList<IVolumeDatapoint>();

		results.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 1L ), "1", "TestVol", "PBYTES", 1.0 ) );
		results.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 2L ), "1", "TestVol", "PBYTES", 1.2 ) );
		results.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 30L ), "1", "TestVol", "PBYTES", 100004.1 ) );
		results.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 15L ), "1", "TestVol", "PBYTES", 10186.33 ) );
		results.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 12L ), "1", "TestVol", "PBYTES", 4006.0 ) );
		results.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 110L ), "1", "TestVol", "PBYTES", 1000000.0 ) );
		results.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 7L ), "1", "TestVol", "PBYTES", 10.1 ) );
		results.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 8L ), "1", "TestVol", "PBYTES", 18.64 ) );
		results.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 11L ), "1", "TestVol", "PBYTES", 33.0 ) );
		results.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 3L ), "1", "TestVol", "PBYTES", 1.9 ) );
		results.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 4L ), "1", "TestVol", "PBYTES", 3.0 ) );
		results.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 5L ), "1", "TestVol", "PBYTES", 4.1 ) );
		results.add( new VolumeDatapoint( TimeUnit.MINUTES.toSeconds( 6L ), "1", "TestVol", "PBYTES", 5.9 ) );
	}

	@Test
	public void testPbytesMax() {

		Series s = SeriesHelper.generate( results, 0L, Metrics.PBYTES, 10L, 100, StatOperation.MAX_X );

		assertEquals( 4, s.getDatapoints().size() );

		Double maxSize = s.getDatapoints().stream().mapToDouble( Datapoint::getY ).max().getAsDouble();
		assertEquals( new Double(1000000.0), maxSize);

		assertEquals( new Double( 1000000.0 ), s.getDatapoints().get( 3 ).getY() );
	}

}

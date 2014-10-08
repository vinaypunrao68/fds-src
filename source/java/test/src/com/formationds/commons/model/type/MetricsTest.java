/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.type;

import com.formationds.commons.model.VolumeDatapoint;
import com.formationds.commons.model.exception.UnsupportedMetricException;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.commons.model.type.Metrics;
import org.json.JSONArray;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import java.util.ArrayList;
import java.util.List;

public class MetricsTest {
  private static final String JSON =
    "[\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"0.0\",\n" +
      "    \"key\": \"Puts\",\n" +
      "    \"timestamp\": \"1412126335\"\n" +
      "  },\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"0.0\",\n" +
      "    \"key\": \"Gets\",\n" +
      "    \"timestamp\": \"1412126335\"\n" +
      "  },\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"0.0\",\n" +
      "    \"key\": \"Queue Full\",\n" +
      "    \"timestamp\": \"1412126335\"\n" +
      "  },\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"0.0\",\n" +
      "    \"key\": \"Percent of SSD Accesses\",\n" +
      "    \"timestamp\": \"1412126335\"\n" +
      "  },\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"0.0\",\n" +
      "    \"key\": \"Logical Bytes\",\n" +
      "    \"timestamp\": \"1412126335\"\n" +
      "  },\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"0.0\",\n" +
      "    \"key\": \"Physical Bytes\",\n" +
      "    \"timestamp\": \"1412126335\"\n" +
      "  },\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"0.0\",\n" +
      "    \"key\": \"Metadata Bytes\",\n" +
      "    \"timestamp\": \"1412126335\"\n" +
      "  },\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"0.0\",\n" +
      "    \"key\": \"Blobs\",\n" +
      "    \"timestamp\": \"1412126335\"\n" +
      "  },\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"0.0\",\n" +
      "    \"key\": \"Objects\",\n" +
      "    \"timestamp\": \"1412126335\"\n" +
      "  },\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"0.0\",\n" +
      "    \"key\": \"Ave Blob Size\",\n" +
      "    \"timestamp\": \"1412126335\"\n" +
      "  },\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"0.0\",\n" +
      "    \"key\": \"Ave Objects per Blob\",\n" +
      "    \"timestamp\": \"1412126335\"\n" +
      "  },\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"0.0\",\n" +
      "    \"key\": \"Short Term Capacity Sigma\",\n" +
      "    \"timestamp\": \"1412126335\"\n" +
      "  },\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"0.0\",\n" +
      "    \"key\": \"Long Term Capacity Sigma\",\n" +
      "    \"timestamp\": \"1412126335\"\n" +
      "  },\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"0.0\",\n" +
      "    \"key\": \"Short Term Capacity WMA\",\n" +
      "    \"timestamp\": \"1412126335\"\n" +
      "  },\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"0.0\",\n" +
      "    \"key\": \"Short Term Perf Sigma\",\n" +
      "    \"timestamp\": \"1412126335\"\n" +
      "  },\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"0.0\",\n" +
      "    \"key\": \"Long Term Perf Sigma\",\n" +
      "    \"timestamp\": \"1412126335\"\n" +
      "  },\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"0.0\",\n" +
      "    \"key\": \"Short Term Perf WMA\",\n" +
      "    \"timestamp\": \"1412126335\"\n" +
      "  },\n" +
      "]";
  private static final List<VolumeDatapoint> DATAPOINTS = new ArrayList<>();

  @Before
  public void setUp()
  {
    final JSONArray array = new JSONArray( JSON );
    for( int i = 0; i < array.length(); i++ ) {
      DATAPOINTS.add( ObjectModelHelper.toObject( array.getJSONObject( i )
                                                       .toString(),
                                                  VolumeDatapoint.class ) );
    }
  }

  @Test
  public void test() {
    for( final VolumeDatapoint vdp : DATAPOINTS )
    {
      try {
        Metrics.byMetadataKey( vdp.getKey() );
      } catch( UnsupportedMetricException e ) {
        e.printStackTrace();
        Assert.fail( e.getMessage() );
      }
    }
  }

  @Test
  public void failTest() {
    try {
      Metrics.byMetadataKey( "Unknown Metric Name" );
      Assert.fail( "Excepted to fail, but was successful!" );
    } catch( UnsupportedMetricException ignore ) {
      // thrown exception is expected in this junit test
    }
  }
}
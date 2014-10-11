/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository;

import com.formationds.commons.model.VolumeDatapoint;
import com.formationds.commons.model.builder.VolumeDatapointBuilder;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.commons.model.type.Metrics;
import org.json.JSONArray;
import org.junit.Assert;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;

public class MetricsRepositoryTest {
  private static final String DB_PATH =
    "/tmp/db/metadata.odb";
  private static final String DB_PATH_TRANS_LOG =
    "/tmp/db/metadata.odb$";

  private static final String JSON =
    "[\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"2.0\",\n" +
      "    \"key\": \"Puts\",\n" +
      "    \"timestamp\": \"1412126335\"\n" +
      "  },\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"5.0\",\n" +
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

  private static
  MetricsRepository store = null;

  public void setUp()
  {
    store = new MetricsRepository( DB_PATH );
    final JSONArray array = new JSONArray( JSON );
    for( int i = 0; i < array.length(); i++ ) {
      DATAPOINTS.add( ObjectModelHelper.toObject( array.getJSONObject( i )
                                                       .toString(),
                                                  VolumeDatapoint.class ) );
    }
  }

  public void create()
  {
    store.save( new VolumeDatapointBuilder().withKey( Metrics.PUTS.key() )
                                            .withVolumeName( "TestVolume" )
                                            .withDateTime( System.currentTimeMillis() / 1000 )
                                            .withValue( 1.0 )
                                            .build() );
    Assert.assertTrue( Files.exists( Paths.get( DB_PATH ) ) );
  }

  public void populate()
  {
    DATAPOINTS.forEach( store::save );
  }

  public void counts()
  {
    System.out.println( "Total row count: " +
                          store.countAll() );

    System.out.println( "TestVolume row count: " +
                          store.countAllBy(
                            new VolumeDatapointBuilder()
                              .withVolumeName( "TestVolume" )
//                              .withKey( Metrics.PUTS.key() )
                              .build() ) );
  }

  public void findAll()
  {
    store.findAll()
         .forEach( System.out::println );
  }

  public void remove()
  {
    System.out.println( "before: " + store.countAll() );
    store.findAll()
         .stream()
         .filter( vdp -> vdp.getKey()
                            .equals( Metrics.LBYTES.key() ) )
         .forEach( store::delete );
    System.out.println( "after: " + store.countAll() );
  }

  public void removeAll()
  {
    System.out.println( "before: " + store.countAll() );
    store.findAll()
         .forEach( store::delete );
    System.out.println( "after: " + store.countAll() );
  }

  public void cleanup()
  {
    try {
      if( Files.exists( Paths.get( DB_PATH ) ) ) {
        Files.delete( Paths.get( DB_PATH ) );
      }

      if( Files.exists( Paths.get( DB_PATH_TRANS_LOG ) ) ) {
        Files.delete( Paths.get( DB_PATH_TRANS_LOG ) );
      }

      if( Files.exists( Paths.get( DB_PATH ).getParent() ) ) {
        Files.delete( Paths.get( DB_PATH )
                           .getParent() );
      }
    } catch( IOException e ) {
      Assert.fail( e.getMessage() );
    }
  }

  /**
   * Since there is a directive inplace that states we shou
   */
//  @Test
//  public void test()
//  {
//    setUp();
//    create();
//    populate();
//    counts();
//    findAll();
//    remove();
//    removeAll();
//    cleanup();
//  }
}
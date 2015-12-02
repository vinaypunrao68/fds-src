/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.webkit.rest.v08.volumes;

import com.formationds.apis.MediaPolicy;
import com.formationds.apis.VolumeDescriptor;
import com.formationds.apis.VolumeSettings;
import com.formationds.apis.VolumeType;
import com.formationds.client.v08.converters.ExternalModelConverter;
import com.formationds.client.v08.model.Size;
import com.formationds.client.v08.model.VolumeState;
import com.formationds.client.v08.model.VolumeStatus;
import com.formationds.commons.togglz.FdsTogglzConfig;
import com.formationds.commons.togglz.feature.FdsFeatureManagerProvider;
import com.formationds.commons.togglz.feature.flag.FdsFeatureToggles;
import com.formationds.om.OmConfigurationApi;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.protocol.IScsiTarget;
import com.formationds.protocol.LogicalUnitNumber;
import com.formationds.protocol.NfsOption;
import com.formationds.protocol.svc.types.ResourceState;
import com.formationds.util.thrift.ConfigurationApi;
import org.junit.After;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;
import org.togglz.core.Feature;
import org.togglz.core.context.FeatureContext;
import org.togglz.core.manager.EnumBasedFeatureProvider;
import org.togglz.core.manager.FeatureManager;
import org.togglz.core.manager.FeatureManagerBuilder;
import org.togglz.core.repository.mem.InMemoryStateRepository;
import org.togglz.core.user.NoOpUserProvider;
import org.togglz.junit.TogglzRule;
import org.togglz.junit.WithFeature;

import java.time.Instant;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static org.mockito.Matchers.anyString;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

/**
 * @author ptinius
 */
@Ignore
public class ListVolumesTest
{
    final SingletonConfigAPI singletonConfigApi = mock( SingletonConfigAPI.class );
    final ConfigurationApi configApi = mock( OmConfigurationApi.class );

    static final List<VolumeDescriptor> volumes = new ArrayList<>( );
    static final Map<Long, VolumeStatus> volumeStats = new HashMap<>( );
    static
    {
        // Block Volume
        final VolumeSettings block = new VolumeSettings( Size.gb( 1000 ).getValue().intValue(),
                                                         VolumeType.BLOCK,
                                                         Size.gb( 1000 ).getValue().longValue(),
                                                         Instant.now().getEpochSecond(),
                                                         MediaPolicy.HYBRID_ONLY );
        final VolumeStatus blockStatus = new VolumeStatus( VolumeState.Active, Size.gb( 1000 ) );
        volumeStats.put( 1L, blockStatus );
        volumes.add( new VolumeDescriptor( "volume1",
                                           Instant.now().getEpochSecond(),
                                           block,
                                           0L,
                                           1L,
                                           ResourceState.Active ) );

        // Object Volume ( S3 )
        final VolumeSettings object = new VolumeSettings( Size.gb( 1000 ).getValue().intValue(),
                                                          VolumeType.BLOCK,
                                                          Size.gb( 1000 ).getValue().longValue(),
                                                          Instant.now().getEpochSecond(),
                                                          MediaPolicy.HYBRID_ONLY );
        final VolumeStatus objectStatus = new VolumeStatus( VolumeState.Active, Size.gb( 1000 ) );
        volumeStats.put( 2L, objectStatus );
        volumes.add( new VolumeDescriptor( "volume2",
                                           Instant.now().getEpochSecond(),
                                           object,
                                           0L,
                                           2L,
                                           ResourceState.Active ) );

        // NFS volume
        final VolumeSettings nfs = new VolumeSettings( Size.gb( 1000 ).getValue().intValue(),
                                                       VolumeType.BLOCK,
                                                       Size.gb( 1000 ).getValue().longValue(),
                                                       Instant.now().getEpochSecond(),
                                                       MediaPolicy.HYBRID_ONLY );
        final NfsOption nfsOption = new NfsOption( );
        nfsOption.addToOptions( "acl" );
        nfsOption.addToOptions( "squash" );
        nfsOption.addToIpfilters( "10.1.10.*" );
        nfsOption.addToIpfilters( "10.2.10.*" );
        nfsOption.addToIpfilters( "10.3.10.10" );
        nfs.setNfsOptions( nfsOption );
        final VolumeStatus nfsStatus = new VolumeStatus( VolumeState.Active, Size.gb( 1000 ) );
        nfs.setNfsOptions( nfsOption );
        volumeStats.put( 3L, nfsStatus );
        volumes.add( new VolumeDescriptor( "volume3",
                                           Instant.now().getEpochSecond(),
                                           nfs,
                                           0L,
                                           3L,
                                           ResourceState.Active ) );

        // iSCSI volume
        final VolumeSettings iscsi = new VolumeSettings( Size.gb( 1000 ).getValue().intValue(),
                                                         VolumeType.BLOCK,
                                                         Size.gb( 1000 ).getValue().longValue(),
                                                         Instant.now().getEpochSecond(),
                                                         MediaPolicy.HYBRID_ONLY );
        final IScsiTarget target = new IScsiTarget();
        final LogicalUnitNumber lun1 = new LogicalUnitNumber( "volume4", "rw" );
        target.setLuns( Collections.singletonList( lun1 ) );
        iscsi.setIscsiTarget( target );
        final VolumeStatus iscsiStatus = new VolumeStatus( VolumeState.Active, Size.gb( 1000 ) );
        volumeStats.put( 4L, iscsiStatus );
        volumes.add( new VolumeDescriptor( "volume4",
                                           Instant.now().getEpochSecond(),
                                           iscsi,
                                           0L,
                                           4L,
                                           ResourceState.Active ) );
    }

    @Before
    public void setUp( )
        throws Exception
    {
        /*
         * TODO revisit why the convert is using static methods! makes mocking very difficult
         */
        when( ExternalModelConverter.loadExternalVolumeStatus( volumes ) )
            .thenReturn( volumeStats );
        when( singletonConfigApi.api() ).thenReturn( configApi );
        when( configApi.listVolumes( anyString() ) ).thenReturn( volumes );
    }

    @After
    public void tearDown( )
        throws Exception
    {
    }

    @Test
    public void testHandler( )
        throws Exception
    {

    }

    @Test
    @WithFeature( "FS_2660_METRIC_FIREBREAK_EVENT_QUERY" )
    public void testListVolumes( )
        throws Exception
    {
        ( new ListVolumes( null, null ) ).listVolumes( );
    }
}

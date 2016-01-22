/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.webkit.rest.v08.volumes;

import com.formationds.client.v08.model.*;
import com.formationds.protocol.ApiException;
import com.google.common.base.CharMatcher;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import java.math.BigDecimal;
import java.time.Instant;
import java.util.HashMap;

/**
 * @author ptinius
 */
public class CreateVolumeTest
{
    Volume nullVolume;
    Volume nullQOSVolume;
    Volume validVolume;

    @Before
    public void setup() {

        final BigDecimal bDec =
            BigDecimal.valueOf( ( ( 1024 * 1024 ) * 1024 ) * 50.0 );

        final Volume baseVolume =
            new Volume.Builder( "BaseVolume" )
                      .id( 0 )
                      .tenant( new Tenant( 0L, "JUNIT" ) )
                      .application( "junit" )
                      .status( new VolumeStatus( VolumeState.Created,
                                                 new Size( bDec,
                                                           SizeUnit.GB ) ) )
                      // settings
                      .mediaPolicy( MediaPolicy.HDD )
                      .dataProtectionPolicy(
                          new DataProtectionPolicyPreset( ) )
                      .accessPolicy( VolumeAccessPolicy.exclusiveRWPolicy( ) )
                      .qosPolicy( new QosPolicy( 3, 0, 2000 ) )
                      .addTags( new HashMap<>( ) )
                      .creationTime( Instant.now( ) )
                      .create( );
        baseVolume.setSettings( new VolumeSettingsObject( ) );

        nullVolume = null;

        nullQOSVolume = new Volume.Builder( baseVolume, "NullQOSVolume" )
                                  .id( 1 )
                                  .create( );
        nullQOSVolume.setQosPolicy( null );

        validVolume = new Volume.Builder( baseVolume, "ValidVolume" )
                                .id( 2 )
                                .create( );
        validVolume.setSettings( new VolumeSettingsObject(  ) );
    }

    @Test( expected = ApiException.class )
    public void testValidateQOSSettingsNullVolume( ) throws Exception {
        ( new CreateVolume( null, null ) ).validateQOSSettings( nullVolume );
    }

    @Test( expected = ApiException.class )
    public void testValidateQOSSettingsNullVolumeQOSPolicy( ) throws Exception {
        ( new CreateVolume( null, null ) ).validateQOSSettings( nullQOSVolume );
    }

    @Test( expected = ApiException.class )
    public void testValidateQOSSettingsCreatedVolumeNullQOSPolicy() throws Exception {
        ( new CreateVolume( null, null ) ).validateQOSSettings( nullQOSVolume,
                                                                true );
    }

    @Test
    public void testValidateCreateVolumeQOSSettings() throws Exception {
        ( new CreateVolume( null, null ) ).validateQOSSettings( validVolume,
                                                                true );
    }

    @Test( expected = ApiException.class )
    public void testValidateCreateVolumeQOSSettingOutOfRange() throws Exception {
        validVolume.setQosPolicy( new QosPolicy( 3, 1500, 1000 ) );
        ( new CreateVolume( null, null ) ).validateQOSSettings( validVolume,
                                                                true );
    }

    @Test( expected = ApiException.class )
    public void testValidateEditVolumeQOSSettingOutOfRange() throws Exception {
        validVolume.setQosPolicy( new QosPolicy( 3, 1500, 1000 ) );
        ( new CreateVolume( null, null ) ).validateQOSSettings( validVolume );
    }

    @Test
    public void testValidateCreateVolumeQOSSettingUnlimited() throws Exception
    {
        validVolume.setQosPolicy( new QosPolicy( 3, 1500, 0 ) );
        ( new CreateVolume( null, null ) ).validateQOSSettings( validVolume,
                                                                true );
    }

    @Test
    public void testVolumeNameWithSpacesRegEx()
    {
        final String volumeName = "This Volume Has Spaces in the name";
        Assert.assertTrue( CharMatcher.WHITESPACE.matchesAnyOf( volumeName ) );
    }
    @Test
    public void testVolumeNameWithoutSpacesRegEx()
    {
        final String volumeName = "TestVolume_Name";
        Assert.assertFalse( CharMatcher.WHITESPACE.matchesAnyOf( volumeName ) );
    }
    @Test
    public void testVolumeNameWithDNSStyleNamesRegEx()
    {
        final String volumeName = "c3po.s3.formationds.com";
        Assert.assertFalse( CharMatcher.WHITESPACE.matchesAnyOf( volumeName ) );
    }
}

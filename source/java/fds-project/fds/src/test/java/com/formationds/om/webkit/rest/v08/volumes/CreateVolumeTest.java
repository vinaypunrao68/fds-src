/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.webkit.rest.v08.volumes;

import com.formationds.client.v08.model.DataProtectionPolicyPreset;
import com.formationds.client.v08.model.MediaPolicy;
import com.formationds.client.v08.model.QosPolicy;
import com.formationds.client.v08.model.Size;
import com.formationds.client.v08.model.SizeUnit;
import com.formationds.client.v08.model.Tenant;
import com.formationds.client.v08.model.Volume;
import com.formationds.client.v08.model.VolumeAccessPolicy;
import com.formationds.client.v08.model.VolumeSettingsObject;
import com.formationds.client.v08.model.VolumeState;
import com.formationds.client.v08.model.VolumeStatus;
import com.formationds.protocol.ApiException;
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
}
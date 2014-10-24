/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository.query.builder;

import com.formationds.commons.model.entity.VolumeDatapoint;

/**
 * @author ptinius
 */
public class VolumeDatapointBuilder {
  private Long timestamp;
  private String volumeName;
  private String key;
  private Double value;

  /**
   * default constructor
   */
  public VolumeDatapointBuilder() {
  }

  /**
   * @param timestamp the {@link Long} representing the timestamp in seconds
   *
   * @return Returns the {@link VolumeDatapointBuilder}
   */
  public VolumeDatapointBuilder withTimestamp( Long timestamp ) {
    this.timestamp = timestamp;
    return this;
  }

  /**
   * @param volumeName the {@link String} representing the volume names
   *
   * @return Returns the {@link VolumeDatapointBuilder}
   */
  public VolumeDatapointBuilder withVolumeName( String volumeName ) {
    this.volumeName = volumeName;
    return this;
  }

  /**
   * @param key the {@link String} representing the key
   *
   * @return Returns the {@link VolumeDatapointBuilder}
   */
  public VolumeDatapointBuilder withKey( String key ) {
    this.key = key;
    return this;
  }

  /**
   * @param value the {@link Double} representing the value
   *
   * @return Returns the {@link VolumeDatapointBuilder}
   */
  public VolumeDatapointBuilder withValue( Double value ) {
    this.value = value;
    return this;
  }

  /**
   * @return Returns the {@link com.formationds.commons.model.entity.VolumeDatapoint}
   */
  public VolumeDatapoint build() {
    VolumeDatapoint volumeDatapoint = new VolumeDatapoint();

    if( volumeName != null ) {
      volumeDatapoint.setVolumeName( volumeName );
    }

    if( key != null ) {
      volumeDatapoint.setKey( key );
    }

    if( timestamp != null ) {
      volumeDatapoint.setTimestamp( timestamp );
    }

    if( value != null ) {
      volumeDatapoint.setValue( value );
    }

    return volumeDatapoint;
  }
}

/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.builder;

import com.formationds.commons.model.VolumeDatapoint;

/**
 * @author ptinius
 */
public class VolumeDatapointBuilder {
  private long dateTime;
  private String volumeName;
  private String key;
  private double value;

  /**
   * default constructor
   */
  public VolumeDatapointBuilder() {
  }

  /**
   * @param dateTime the {@code long} representing the date/time in seconds
   *
   * @return Returns the {@link com.formationds.commons.model.builder.VolumeDatapointBuilder}
   */
  public VolumeDatapointBuilder withDateTime( long dateTime ) {
    this.dateTime = dateTime;
    return this;
  }

  /**
   * @param volumeName the {@link String} representing the volume names
   *
   * @return Returns the {@link com.formationds.commons.model.builder.VolumeDatapointBuilder}
   */
  public VolumeDatapointBuilder withVolumeName( String volumeName ) {
    this.volumeName = volumeName;
    return this;
  }

  /**
   * @param key the {@link String} representing the key
   *
   * @return Returns the {@link com.formationds.commons.model.builder.VolumeDatapointBuilder}
   */
  public VolumeDatapointBuilder withKey( String key ) {
    this.key = key;
    return this;
  }

  /**
   * @param value the {@code double} representing thevalue
   *
   * @return Returns the {@link com.formationds.commons.model.builder.VolumeDatapointBuilder}
   */
  public VolumeDatapointBuilder withValue( double value ) {
    this.value = value;
    return this;
  }

  /**
   * @return Returns the {@link com.formationds.commons.model.VolumeDatapoint}
   */
  public VolumeDatapoint build() {
    VolumeDatapoint volumeDatapoint = new VolumeDatapoint();
    volumeDatapoint.setDateTime( dateTime );
    volumeDatapoint.setVolumeName( volumeName );
    volumeDatapoint.setKey( key );
    volumeDatapoint.setValue( value );
    return volumeDatapoint;
  }
}

/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.entity;

import com.formationds.commons.model.abs.ModelBase;
import com.google.gson.annotations.SerializedName;

import javax.persistence.Entity;
import javax.persistence.GeneratedValue;

/**
 * @author ptinius
 */
@Entity
public class VolumeDatapoint
  extends ModelBase {
  private static final long serialVersionUID = -528746171551767393L;

  @GeneratedValue
  private Integer id;
  @SerializedName("timestamp")
  private Long timestamp;
  @SerializedName("volume")
  private String volumeName;
  @SerializedName( "volumeId" )
  private String volumeId;
  private String key;
  private Double value;

  public VolumeDatapoint() {
  }

  /**
   * @return Returns the auto-generated id
   */
  public Integer getId() {
    return id;
  }

  /**
   * @return Returns the {@code double} representing the statistic value for
   * the {@code key}
   */
  public Double getValue() {
    return value;
  }

  /**
   * @param value the {@code double} representing the statistic value for the
   *              {@code key}
   */
  public void setValue( final Double value ) {
    this.value = value;
  }

  /**
   * @return Returns the {@code long} representing the timestamp
   */
  public Long getTimestamp() {
    return timestamp;
  }

  /**
   * @param timestamp the {@code long} representing the timestamp
   */
  public void setTimestamp( final Long timestamp ) {
    this.timestamp = timestamp;
  }

  /**
   * @return Returns the {@link String} representing the volume name
   */
  public String getVolumeName() {
    return volumeName;
  }

  /**
   * @param volumeName the {@link String} representing the volume name
   */
  public void setVolumeName( final String volumeName ) {
    this.volumeName = volumeName;
  }

  /**
   * @return Returns {@link String} representing the volume id
   */
  public String getVolumeId() {
    return volumeId;
  }

  /**
   * @param volumeId the {@link String} representing the volume id
   */
  public void setVolumeId( final String volumeId ) {
    this.volumeId = volumeId;
  }

  /**
   * @return Returns the {@link String} representing the metadata key
   */
  public String getKey() {
    return key;
  }

  /**
   * @param key the {@link String} representing the metadata key
   */
  public void setKey( final String key ) {
    this.key = key;
  }
}

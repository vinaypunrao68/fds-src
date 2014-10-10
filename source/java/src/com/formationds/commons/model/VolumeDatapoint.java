/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 *
 * This software is furnished under a license and may be used and copied only
 * in  accordance  with  the  terms  of such  license and with the inclusion
 * of the above copyright notice. This software or  any  other copies thereof
 * may not be provided or otherwise made available to any other person.
 * No title to and ownership of  the  software  is  hereby transferred.
 *
 * The information in this software is subject to change without  notice
 * and  should  not be  construed  as  a commitment by Formation Data Systems.
 *
 * Formation Data Systems assumes no responsibility for the use or  reliability
 * of its software on equipment which is not supplied by Formation Date Systems.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;
import com.google.gson.annotations.SerializedName;

import javax.persistence.Entity;
import javax.persistence.GeneratedValue;
import javax.xml.bind.annotation.XmlRootElement;

/**
 * @author ptinius
 */
@XmlRootElement
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
  private String key;
  private Double value;

  /**
   * @return Returns the {@code double} representing the statistic value for the {@code key}
   */
  public Double getValue() {
    return value;
  }

  /**
   * @param value the {@code double} representing the statistic value for the {@code key}
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

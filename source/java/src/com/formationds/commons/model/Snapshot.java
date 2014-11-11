/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;

import java.util.Date;

/**
 * @author ptinius
 */
public class Snapshot
  extends ModelBase {
  private static final long serialVersionUID = 7430533053472660819L;

  private String id;
  private String name;
  private String volumeId;
  private Date creation;
  private Long retention;
  private Long timelineTime;

  /**
   * default constructor
   */
  public Snapshot() {
    super();
  }

  /**
   * @return Returns the {@link String} representing the snapshot id
   */
  public String getId() {
    return id;
  }

  /**
   * @param id the {@link String} representing the snapshot id
   */
  public void setId( final String id ) {
    this.id = id;
  }

  /**
   * @return Returns the {@link String} representing the tenants name
   */
  public String getName() {
    return name;
  }

  /**
   * @param name the {@link String} representing the tenants name
   */
  public void setName( final String name ) {
    this.name = name;
  }

  /**
   * @return Returns {@link String} representing the volume id
   */
  public String getVolumeId() {
    return volumeId;
  }

  /**
   * @return Returns {@link String} representing the time line time
   */
  public long getTimelineTime() {
    return timelineTime;
  }

  /**
   * @param volumeId the {@link String} representing the volume id:w
   */
  public void setVolumeId( final String volumeId ) {
    this.volumeId = volumeId;
  }

  /**
   * @return Returns {@link Date} representing the creation date of this
   * snapshot
   */
  public Date getCreation() {
    return creation;
  }

  /**
   * @param creation the {@link Date} representing the creation date of this
   *                 snapshot
   */
  public void setCreation( final Date creation ) {
    this.creation = creation;
  }

  /**
   * @return Returns {@link Long} representing the retention time
   */
  public Long getRetention() {
    return retention;
  }

  /**
   * @param retention the {@link Long} representing the retention time
   */
  public void setRetention( Long retention ) {
    this.retention = retention;
  }

  /**
   * @param retention the {@link Long} representing the time line time 
   */
  public void setTimelineTime( Long timelineTime ) {
    this.timelineTime = timelineTime;
  }
}

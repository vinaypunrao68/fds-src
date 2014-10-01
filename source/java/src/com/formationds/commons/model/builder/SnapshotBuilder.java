/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.builder;

import com.formationds.commons.model.Snapshot;

import java.util.Date;

/**
 * @author ptinius
 */
public class SnapshotBuilder {
  private Long id;
  private String name;
  private Long volumeId;
  private Date creation;
  private Long retention;

  /**
   * default constructor
   */
  public SnapshotBuilder() {
  }

  /**
   * @param id the {@code long} representing the universal unique identifier
   *
   * @return Returns the {@link SnapshotBuilder}
   */
  public SnapshotBuilder withId( Long id ) {
    this.id = id;
    return this;
  }

  /**
   * @param name the {@link String} representing the snapshot name
   *
   * @return Returns the {@link SnapshotBuilder}
   */
  public SnapshotBuilder withName( String name ) {
    this.name = name;
    return this;
  }

  /**
   * @param volumeId the {@code long} representing the volume identifier
   *
   * @return Returns the {@link SnapshotBuilder}
   */
  public SnapshotBuilder withVolumeId( Long volumeId ) {
    this.volumeId = volumeId;
    return this;
  }

  /**
   * @param creation {@link Date}
   *
   * @return Returns the {@link SnapshotBuilder}
   */
  public SnapshotBuilder withCreation( Date creation ) {
    this.creation = creation;
    return this;
  }

  /**
   * @param retention the {@code long} representing the retention
   *
   * @return Returns the {@link SnapshotBuilder}
   */
  public SnapshotBuilder withRetention( Long retention ) {
    this.retention = retention;
    return this;
  }

  /**
   * @return Returns the {@link SnapshotBuilder}
   */
  public Snapshot build() {
    Snapshot snapshot = new Snapshot();
    snapshot.setId( id );
    snapshot.setName( name );
    snapshot.setVolumeId( volumeId );
    snapshot.setCreation( creation );
    snapshot.setRetention( retention );

    return snapshot;
  }
}

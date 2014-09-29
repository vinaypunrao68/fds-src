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
  private long id;
  private String name;
  private long volumeId;
  private Date creation;
  private long retention;

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
  public SnapshotBuilder withId( long id ) {
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
  public SnapshotBuilder withVolumeId( long volumeId ) {
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
   * @param creation {@link Date}
   *
   * @return Returns the {@link SnapshotBuilder}
   */
  public SnapshotBuilder withRetention( long creation ) {
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

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
   * @return Returns the {@link SnapshotBuilder}
   */
  public Snapshot build() {
    Snapshot snapshot = new Snapshot();
    snapshot.setId( id );
    snapshot.setName( name );
    snapshot.setVolumeId( volumeId );
    snapshot.setCreation( creation );

    return snapshot;
  }
}

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

import com.formationds.commons.model.RecurrenceRule;
import com.formationds.commons.model.SnapshotPolicy;

/**
 * @author ptinius
 */
public class SnapshotPolicyBuilder {
  private long id;
  private String name;
  private RecurrenceRule recurrenceRule;
  private long retention;                     // time in seconds

  /**
   * default constructor
   */
  public SnapshotPolicyBuilder() {
  }

  /**
   * @param id the {@code int} representing the snapshot policy id
   *
   * @return Returns {@link com.formationds.commons.model.builder.SnapshotBuilder}
   */
  public SnapshotPolicyBuilder withId(final long id) {
    this.id = id;
    return this;
  }

  /**
   * @param name the {@link String} representing the snapshot policies name
   *
   * @return Returns {@link com.formationds.commons.model.builder.SnapshotBuilder}
   */
  public SnapshotPolicyBuilder withName(final String name) {
    this.name = name;
    return this;
  }

  /**
   * @param recurrenceRule the {@link RecurrenceRule} representing the snapshot policy id
   *
   * @return Returns {@link com.formationds.commons.model.builder.SnapshotBuilder}
   */
  public SnapshotPolicyBuilder withRecurrenceRule(final RecurrenceRule recurrenceRule) {
    this.recurrenceRule = recurrenceRule;
    return this;
  }

  /**
   * @param retention the {@code long} representing the snapshot policy retention
   *
   * @return Returns {@link com.formationds.commons.model.builder.SnapshotBuilder}
   */
  public SnapshotPolicyBuilder withRetention(final long retention) {
    this.retention = retention;
    return this;
  }

  /**
   * @return Returns {@link SnapshotPolicy}
   */
  public SnapshotPolicy build() {
    SnapshotPolicy snapshotPolicy = new SnapshotPolicy();
    snapshotPolicy.setId(id);
    snapshotPolicy.setName(name);
    snapshotPolicy.setRecurrenceRule(recurrenceRule);
    snapshotPolicy.setRetention(retention);
    return snapshotPolicy;
  }
}

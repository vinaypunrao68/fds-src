/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;

import java.text.ParseException;

/**
 * @author ptinius
 */
public class SnapshotPolicy
  extends ModelBase {
  private static final long serialVersionUID = -1485532787782450871L;

  private Long id;
  private String name;
  private RecurrenceRule recurrenceRule;
  private Long retention;         // time in seconds

  /**
   * default constructor
   */
  public SnapshotPolicy() {
    super();
  }

  /**
   * @return Returns the {@code long} representing the snapshot policy id
   */
  public Long getId() {
    return id;
  }

  /**
   * @param id the {@code long} representing the snapshot policy id
   */
  public void setId( final Long id ) {
    this.id = id;
  }

  /**
   * @return Returns {@link String} representing the name of the snapshot
   * policy
   */
  public String getName() {
    return name;
  }

  /**
   * @param name the {@link String} representing the name of the snapshot
   *             policy
   */
  public void setName( final String name ) {
    this.name = name;
  }

  /**
   * @return Returns the {@link String}
   */
  public RecurrenceRule getRecurrenceRule() {
    return recurrenceRule;
  }

  /**
   * @param recurrenceRule the {@link String}
   */
  public void setRecurrenceRule( final String recurrenceRule )
    throws ParseException {
    this.recurrenceRule = new RecurrenceRule().parser( recurrenceRule );
  }

  /**
   * @return Returns {@code long} representing the retention period in seconds
   */
  public Long getRetention() {
    return retention;
  }

  /**
   * @param retention the {@code long} representing the retention period in
   *                  seconds
   */
  public void setRetention( final Long retention ) {
    this.retention = retention;
  }
}

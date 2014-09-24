/*
 * Copyright (C) 2014, All Rights Reserved, by Formation Data Systems, Inc.
 *
 *  This software is furnished under a license and may be used and copied only
 *  in  accordance  with  the  terms  of such  license and with the inclusion
 *  of the above copyright notice. This software or  any  other copies thereof
 *  may not be provided or otherwise made available to any other person.
 *  No title to and ownership of  the  software  is  hereby transferred.
 *
 *  The information in this software is subject to change without  notice
 *  and  should  not be  construed  as  a commitment by Formation Data Systems.
 *
 *  Formation Data Systems assumes no responsibility for the use or  reliability
 *  of its software on equipment which is not supplied by Formation Date Systems.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;
import com.formationds.commons.model.exception.ParseException;

import javax.xml.bind.annotation.XmlRootElement;

/**
 * @author ptinius
 */
@XmlRootElement
public class SnapshotPolicy
  extends ModelBase
{
  private long id;
  private String name;
  private RecurrenceRule recurrenceRule;
  private long retention;         // time in seconds

  /**
   * default constructor
   */
  public SnapshotPolicy()
  {
    super();
  }

  /**
   * @return Returns the {@code long} representing the snapshot policy id
   */
  public long getId()
  {
    return id;
  }

  /**
   * @param id the {@code long} representing the snapshot policy id
   */
  public void setId( final long id )
  {
    this.id = id;
  }

  /**
   * @return Returns {@link String} representing the name of the snapshot policy
   */
  public String getName()
  {
    return name;
  }

  /**
   * @param name the {@link String} representing the name of the snapshot policy
   */
  public void setName( final String name )
  {
    this.name = name;
  }

  /**
   * @return Returns the {@link String}
   */
  public RecurrenceRule getRecurrenceRule()
  {
    return recurrenceRule;
  }

  /**
   * @param recurrenceRule the {@link String}
   */
  public void setRecurrenceRule( final String recurrenceRule )
    throws ParseException {
    this.recurrenceRule = RecurrenceRule.parser( recurrenceRule );
  }

  /**
   * @param recurrenceRule the {@link RecurrenceRule}
   */
  public void setRecurrenceRule( final RecurrenceRule recurrenceRule ) {
    this.recurrenceRule = recurrenceRule;
  }

  /**
   * @return Returns {@code long} representing the retention period in seconds
   */
  public long getRetention()
  {
    return retention;
  }

  /**
   * @param retention the {@code long} representing the retention period in seconds
   */
  public void setRetention( final long retention )
  {
    this.retention = retention;
  }
}

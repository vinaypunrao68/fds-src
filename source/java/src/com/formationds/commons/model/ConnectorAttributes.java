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
import com.formationds.commons.model.type.SizeUnit;

import javax.xml.bind.annotation.XmlRootElement;

/**
 * @author ptinius
 */
@XmlRootElement
public class ConnectorAttributes
  extends ModelBase
{
  private SizeUnit unit;
  private long size;

  /**
   * default constructor
   */
  public ConnectorAttributes()
  {
    super();
  }

  /**
   * @return Returns the {@link SizeUnit} representing the {@code units} of the {@link #getSize()}
   */
  public SizeUnit getUnit()
  {
    return unit;
  }

  /**
   * @param unit the {@link SizeUnit} representing the {@code units} of the {@link #getSize()}
   */
  public void setUnit( final SizeUnit unit )
  {
    this.unit = unit;
  }

  /**
   * @return Returns {@code long} representing the size
   */
  public long getSize()
  {
    return size;
  }

  /**
   * @param size the {@code long} representing the size
   */
  public void setSize( final long size )
  {
    this.size = size;
  }
}

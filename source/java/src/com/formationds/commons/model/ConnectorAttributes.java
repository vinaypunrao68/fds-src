/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;
import com.formationds.util.SizeUnit;

import javax.xml.bind.annotation.XmlRootElement;

/**
 * @author ptinius
 */
@XmlRootElement
public class ConnectorAttributes
  extends ModelBase
{
  private static final long serialVersionUID = 4629648232746455453L;

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

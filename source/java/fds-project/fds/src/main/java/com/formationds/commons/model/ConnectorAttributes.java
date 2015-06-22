/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;
import com.formationds.util.SizeUnit;

/**
 * @author ptinius
 */
public class ConnectorAttributes
  extends ModelBase {
  private static final long serialVersionUID = 4629648232746455453L;

  private SizeUnit unit;
  private Long size;

  /**
   * default constructor
   */
  public ConnectorAttributes() {
    super();
  }

  /**
   * @return Returns the {@link SizeUnit} representing the {@code units} of the
   * {@link #getSize()}
   */
  public SizeUnit getUnit() {
    return unit;
  }

  /**
   * @param unit the {@link SizeUnit} representing the {@code units} of the
   *             {@link #getSize()}
   */
  public void setUnit( final SizeUnit unit ) {
    this.unit = unit;
  }

  /**
   * @return Returns {@link Long} representing the size
   */
  public Long getSize() {
    return size;
  }

  /**
   * @param size the {@link Long} representing the size
   */
  public void setSize( final Long size ) {
    this.size = size;
  }
}

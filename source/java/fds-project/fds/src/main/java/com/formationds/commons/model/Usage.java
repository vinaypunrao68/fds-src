/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;
import com.formationds.util.SizeUnit;

/**
 * @author ptinius
 */
public class Usage
  extends ModelBase {
  private static final long serialVersionUID = -2938435229361709388L;

  private SizeUnit unit;
  private String size;

  /**
   * default package level constructor
   */
  public Usage() {
    super();
  }

  /**
   * Property bound setter for {@code size}.
   */
  public void setSize( String size ) {
    ;
    this.size = size;
  }

  /**
   * Property bound setter for {@code size}.
   */
  public String getSize() {
    return size;
  }

  /**
   * Property bound setter for {@code unit}.
   */
  public void setUnit( SizeUnit unit ) {
    this.unit = unit;
  }

  /**
   * Property bound setter for {@code unit}.
   */
  public SizeUnit getUnit() {
    return unit;
  }
}

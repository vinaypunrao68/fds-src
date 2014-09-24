/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.builder;

import com.formationds.commons.model.Usage;
import com.formationds.util.SizeUnit;

/**
 * @author ptinius
 */
public class UsageBuilder {
  private SizeUnit unit;
  private String size;

  /**
   * default constructor
   */
  public UsageBuilder() {
  }

  /**
   * @param unit the {@link SizeUnit} representing the unit of the {@code size}
   *
   * @return Returns the {@link UsageBuilder}
   */
  public UsageBuilder withUnit( SizeUnit unit ) {
    this.unit = unit;
    return this;
  }

  /**
   * @param size the {@link String} representing the size
   *
   * @return Returns the {@link UsageBuilder}
   */
  public UsageBuilder withSize( String size ) {
    this.size = size;
    return this;
  }

  /**
   * @return Returns {@link com.formationds.commons.model.Usage}
   */
  public Usage build() {
    Usage usage = new Usage();
    usage.setUnit( unit );
    usage.setSize( size );
    return usage;
  }
}

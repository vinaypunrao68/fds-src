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

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

import com.formationds.commons.model.ConnectorAttributes;
import com.formationds.util.SizeUnit;

/**
 * @author ptinius
 */
public class ConnectorAttributesBuilder {
  private SizeUnit unit;
  private long size;

  /**
   * default constructor
   */
  public ConnectorAttributesBuilder() {
  }

  /**
   * @param unit the {@link com.formationds.util.Size}
   *
   * @return Returns the {@link ConnectorAttributesBuilder}
   */
  public ConnectorAttributesBuilder withUnit( SizeUnit unit ) {
    this.unit = unit;
    return this;
  }

  /**
   * @param size the {@code long} representing the size
   *
   * @return Returns the {@link ConnectorAttributesBuilder}
   */
  public ConnectorAttributesBuilder withSize( long size ) {
    this.size = size;
    return this;
  }

  /**
   * @return Returns the {@link ConnectorAttributes}
   */
  public ConnectorAttributes build() {
    ConnectorAttributes connectorAttributes = new ConnectorAttributes();
    connectorAttributes.setUnit( unit );
    connectorAttributes.setSize( size );
    return connectorAttributes;
  }
}

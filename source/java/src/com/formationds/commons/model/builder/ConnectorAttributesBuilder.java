/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
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

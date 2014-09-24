/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.builder;

import com.formationds.commons.model.Connector;
import com.formationds.commons.model.ConnectorAttributes;
import com.formationds.commons.model.type.DataConnectorType;

/**
 * @author ptinius
 */
public class ConnectorBuilder {
  private DataConnectorType type = DataConnectorType.UNKNOWN;
  private ConnectorAttributes attributes;

  /**
   * default constructor
   */
  public ConnectorBuilder() {
  }

  /**
   * @param type the {@link DataConnectorType}
   *
   * @return Returns {@link ConnectorBuilder}
   */
  public ConnectorBuilder withType( DataConnectorType type ) {
    this.type = type;
    return this;
  }

  /**
   * @param attributes the {@link ConnectorAttributes}
   *
   * @return Returns {@link ConnectorBuilder}
   */
  public ConnectorBuilder withAttributes( ConnectorAttributes attributes ) {
    this.attributes = attributes;
    return this;
  }

  /**
   * @return Returns {@link ConnectorBuilder}
   */
  public Connector build() {
    Connector connector = new Connector();
    connector.setType( type.name() );
    connector.setAttributes( attributes );
    return connector;
  }
}

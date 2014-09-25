/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.builder;

import com.formationds.commons.model.Connector;
import com.formationds.commons.model.ConnectorAttributes;
import com.formationds.commons.model.type.ConnectorType;

/**
 * @author ptinius
 */
public class ConnectorBuilder {
  private ConnectorType type;
  private ConnectorAttributes attributes;
  private String api;

  /**
   * default constructor
   */
  public ConnectorBuilder() {
  }

  /**
   * @param type the {@link com.formationds.commons.model.type.ConnectorType}
   *
   * @return Returns {@link ConnectorBuilder}
   */
  public ConnectorBuilder withType( ConnectorType type ) {
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
   * @param api the {@link String} representing the supported protocols
   *
   * @return Returns {@link ConnectorBuilder}
   */
  public ConnectorBuilder withApi( String api ) {
    this.api = api;
    return this;
  }

  /**
   * @return Returns {@link ConnectorBuilder}
   */
  public Connector build() {
    Connector connector = new Connector();
    connector.setType( type );
    connector.setAttributes( attributes );
    connector.setApi( api );
    return connector;
  }
}

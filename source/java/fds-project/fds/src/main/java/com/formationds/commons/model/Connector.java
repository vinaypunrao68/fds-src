/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;
import com.formationds.commons.model.type.ConnectorType;
import com.google.gson.annotations.SerializedName;

/**
 * @author ptinius
 */
public class Connector
  extends ModelBase {
  private static final long serialVersionUID = -6902011114156588870L;

  @SerializedName( "type" )
  private ConnectorType type;
  private ConnectorAttributes attributes;
  private String api;

  /**
   * default package level constructor
   */
  public Connector() {
    super();
  }

  /**
   * @return Returns the {@link ConnectorType}
   */
  public ConnectorType getType() {
    return type;
  }

  /**
   * @param type the {@link ConnectorType}
   */
  public void setType( final ConnectorType type ) {
    this.type = type;
  }

  /**
   * @return Returns {@link ConnectorAttributes}
   */
  public ConnectorAttributes getAttributes() {
    return attributes;
  }

  /**
   * @param attributes the {@link ConnectorAttributes}
   */
  public void setAttributes( final ConnectorAttributes attributes ) {
    this.attributes = attributes;
  }

  /**
   * @return Returns {@link String} representing the supported protocols
   */
  public String getApi() {
    return api;
  }

  /**
   * @param api the {@link String} representing the supported protocols
   */
  public void setApi( String api ) {
    this.api = api;
  }
}

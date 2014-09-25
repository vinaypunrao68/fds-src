/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;
import com.formationds.commons.model.type.DataConnectorType;

import javax.xml.bind.annotation.XmlRootElement;

/**
 * @author ptinius
 */
@XmlRootElement
public class Connector
  extends ModelBase
{
  private static final long serialVersionUID = -6902011114156588870L;

  private DataConnectorType type = DataConnectorType.UNKNOWN;
  private ConnectorAttributes attributes;

  /**
   * default package level constructor
   */
  public Connector()
  {
    super();
  }

  /**
   * @return Returns the {@link com.formationds.commons.model.type.DataConnectorType}
   */
  public DataConnectorType getType()
  {
    return type;
  }

  /**
   * @param type the {@link com.formationds.commons.model.type.DataConnectorType}
   */
  public void setType( final String type )
  {
    this.type = DataConnectorType.lookup( type );
  }

  /**
   * @return Returns {@link ConnectorAttributes}
   */
  public ConnectorAttributes getAttributes()
  {
    return attributes;
  }

  /**
   * @param attributes the {@link ConnectorAttributes}
   */
  public void setAttributes( final ConnectorAttributes attributes )
  {
    this.attributes = attributes;
  }
}

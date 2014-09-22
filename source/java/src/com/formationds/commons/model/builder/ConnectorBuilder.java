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
   * static utility method constructor
   */
  private ConnectorBuilder() {
  }

  /**
   * @return Returns {@link ConnectorBuilder}
   */
  public static ConnectorBuilder aConnector() {
    return new ConnectorBuilder();
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
  public ConnectorBuilder but() {
    return aConnector().withType( type )
                       .withAttributes( attributes );
  }

  /**
   * @return Returns {@link ConnectorBuilder}
   */
  public Connector build() {
    Connector connector = new Connector();
    connector.setType( type.getLocalizedName() );
    connector.setAttributes( attributes );
    return connector;
  }
}

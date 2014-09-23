/*
 * Copyright (C) 2014, All Rights Reserved, by Formation Data Systems, Inc.
 *
 *  This software is furnished under a license and may be used and copied only
 *  in  accordance  with  the  terms  of such  license and with the inclusion
 *  of the above copyright notice. This software or  any  other copies thereof
 *  may not be provided or otherwise made available to any other person.
 *  No title to and ownership of  the  software  is  hereby transferred.
 *
 *  The information in this software is subject to change without  notice
 *  and  should  not be  construed  as  a commitment by Formation Data Systems.
 *
 *  Formation Data Systems assumes no responsibility for the use or  reliability
 *  of its software on equipment which is not supplied by Formation Date Systems.
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

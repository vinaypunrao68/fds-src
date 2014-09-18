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

package com.formationds.commons.model.type;

import com.formationds.commons.model.i18n.ModelResource;

/**
 * @author ptinius
 */
public enum DataConnectorType
{
  OBJECT( ModelResource.getString( "data.connector.object.type" ) ),
  BLOCK( ModelResource.getString( "data.connector.block.type" ) ),
  UNKNOWN( ModelResource.getString( "data.connector.unknown.type" ) );

  private final String localized;

  /**
   * @param localized the {@link String} representing the localized name of the type.
   */
  DataConnectorType( final String localized )
  {
    this.localized = localized;
  }

  /**
   * @return Returns the displayable name
   */
  public String getLocalizedName() { return localized; }

  /**
   * @param name the {@link String} representing the name of the data connector type
   *
   * @return Returns {@link DataConnectorType} representing the data connector type
   */
  public static DataConnectorType lookup( final String name )
  {
    for( final DataConnectorType type : values() )
    {
      if( type.name().equalsIgnoreCase( name ) )
      {
        return type;
      }
    }

    return UNKNOWN;
  }
}

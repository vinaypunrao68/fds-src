/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
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

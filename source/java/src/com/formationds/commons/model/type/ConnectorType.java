/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.type;

import com.formationds.commons.model.i18n.ModelResource;
import com.formationds.commons.model.util.FDSStringUtil;

/**
 * @author ptinius
 */
public enum ConnectorType {
  OBJECT( ModelResource.getString( "connector.object.type" ) ),
  BLOCK( ModelResource.getString( "connector.block.type" ) ),
  UNKNOWN( ModelResource.getString( "connector.unknown.type" ) );

  private final String localized;

  /**
   * @param localized the {@link String} representing the localized name of the
   *                  type.
   */
  ConnectorType( final String localized ) {
    this.localized = localized;
  }

  /**
   * @return Returns the displayable name
   */
  public String getLocalizedName() {
    return localized;
  }

  /**
   * @return Returns {@link String} with the first letter capitalized
   */
  public String getName() {
    return FDSStringUtil.UppercaseFirstLetters( name() );
  }

  /**
   * @param name the {@link String} representing the name of the data connector
   *             type
   *
   * @return Returns {@link ConnectorType} representing the data connector type
   */
  public static ConnectorType lookup( final String name ) {
    for( final ConnectorType type : values() ) {
      if( type.name()
              .equalsIgnoreCase( name ) ) {
        return type;
      }
    }

    return UNKNOWN;
  }
}

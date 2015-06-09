/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.type;

/**
 * @author ptinius
 */
public enum IdentityType
{
  // Global Solution Administrator
  ADMIN,
  // TODO allows for each tenant to administer their users
  // Tenant Administrator
  TENANT,
  USER,
  UNKNOWN;

  /**
   * @param name the {@link String} representing the identity to lookup
   *
   * @return Returns {@link com.formationds.commons.model.type.IdentityType}
   */
  public static IdentityType lookUp( final String name )
  {
    for( final IdentityType type : values() )
    {
      if( type.name().equalsIgnoreCase( name ) )
      {
        return type;
      }
    }

    return UNKNOWN;
  }
}

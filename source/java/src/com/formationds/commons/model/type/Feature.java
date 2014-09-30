/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.type;

import com.formationds.commons.model.i18n.ModelResource;

import java.util.ArrayList;
import java.util.List;

/**
 * @author ptinius
 */
public enum Feature {
  SYS_MGMT( ModelResource.getString( "fds.feature.system.management" ),
            IdentityType.ADMIN ),
  VOL_MGMT( ModelResource.getString( "fds.feature.volume.management" ),
            IdentityType.USER ),
  TENANT_MGMT( ModelResource.getString( "fds.feature.tenant.management" ),
               IdentityType.TENANT ),
  USER_MGMT( ModelResource.getString( "fds.feature.user.management" ),
             IdentityType.USER ),
  UNKNOWN( "Unknown", IdentityType.UNKNOWN );


  private final String localized;
  private final IdentityType type;

  /**
   * @param localized the {@link String} representing the localized feature
   *                  name
   * @param type      the {@link IdentityType} representing the access to the
   *                  feature
   */
  Feature( final String localized, final IdentityType type ) {
    this.localized = localized;
    this.type = type;
  }

  /**
   * @return Return {@link String} representing the localized feature name
   */
  public String getLocalized() {
    return localized;
  }

  /**
   * @return Returns {@link IdentityType} representing the access to the
   * feature
   */
  public IdentityType getType() {
    return type;
  }

  /**
   * @param name the lookup of {@link #name()}
   *
   * @return Returns {@link Feature}
   */
  public static Feature lookup( final String name ) {
    for( final Feature feature : values() ) {
      if( feature.name().equalsIgnoreCase( name ) ||
         feature.getLocalized().equalsIgnoreCase( name ) ) {
        return feature;
      }
    }

    return UNKNOWN;
  }

  /**
   * @param type the {@link IdentityType}
   *
   * @return Returns {@link List} of {@link Feature} available to the
   *         IdentityType
   */
  public static List<Feature> byRole( final IdentityType type ) {
    final List<Feature> features = new ArrayList<>();

    switch( type.name() )
    {
      case "ADMIN":
        features.add( SYS_MGMT );
      case "TENANT":
        features.add( TENANT_MGMT );
      case "USER":
        features.add( VOL_MGMT );
        features.add( USER_MGMT );
    }

    return features;
  }
}

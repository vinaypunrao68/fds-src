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

import java.util.ArrayList;
import java.util.List;

/**
 * @author ptinius
 */
public enum Feature
{
  SYS_MGMT( ModelResource.getString( "fds.feature.system.management" ),
           IdentityType.ADMIN ),
  VOL_MGMT( ModelResource.getString( "fds.feature.volume.management" ),
            IdentityType.USER ),
  TENANT_MGMT( ModelResource.getString( "fds.feature.tenant.management" ),
               IdentityType.ADMIN ),
  USER_MGMT( ModelResource.getString( "fds.feature.user.management" ),
             IdentityType.USER ),
  UNKNOWN( "Unknown", IdentityType.UNKNOWN );


  private final String localized;
  private final IdentityType type;

  /**
   * @param localized the {@link String} representing the localized feature name
   * @param type the {@link IdentityType} representing the access to the feature
   */
  Feature( final String localized, final IdentityType type )
  {
    this.localized = localized;
    this.type = type;
  }

  /**
   * @return Return {@link String} representing the localized feature name
   */
  public String getLocalized()
  {
    return localized;
  }

  /**
   * @return Returns {@link IdentityType} representing the access to the feature
   */
  public IdentityType getType()
  {
    return type;
  }

  /**
   * @param name the lookup of {@link #name()}
   *
   * @return Returns {@link Feature}
   */
  public static Feature lookup( final String name )
  {
    for( final Feature feature : values() )
    {
      if( feature.name().equalsIgnoreCase( name ) ||
          feature.getLocalized().equalsIgnoreCase( name ) )
      {
        return feature;
      }
    }

    return UNKNOWN;
  }

  /**
   * @param type the {@link IdentityType}
   *
   * @return Returns {@link List} of {@link Feature} available to the IdentityType
   */
  public static List<Feature> byRole( final IdentityType type )
  {
    final List<Feature> features = new ArrayList<>( );

    for( final Feature feature : values() )
    {
      if( feature.getType().equals( type ) )
      {
        features.add( feature );
      }
    }

    return features;
  }
}

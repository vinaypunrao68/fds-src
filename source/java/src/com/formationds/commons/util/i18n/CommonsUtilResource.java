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

package com.formationds.commons.util.i18n;

import java.util.MissingResourceException;
import java.util.ResourceBundle;

/**
 * @author ptinius
 */
public class CommonsUtilResource
{
  private static final String BUNDLE_NAME = "fds-commons-util";

  private static final ResourceBundle BUNDLE =
    ResourceBundle.getBundle( BUNDLE_NAME );

  /**
   * @param key the resource key
   * @return the string associated with the given resource key.
   */
  public static String getString( final String key )
  {
    return BUNDLE.getString( key );
  }

  /**
   * @param key the resource key
   * @param def the default value if the key isn't located
   * @return the string associated with the given resource key.
   */
  public static String getString( final String key, final String def )
  {
    try
    {
      return BUNDLE.getString( key );
    }
    catch( MissingResourceException e )
    {
      return def;
    }
  }

  private CommonsUtilResource()
  {
    super();
  }

}

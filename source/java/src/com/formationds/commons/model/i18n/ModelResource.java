/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.i18n;

import java.util.MissingResourceException;
import java.util.ResourceBundle;

/**
 * @author ptinius
 */
public class ModelResource
{
  private static final String BUNDLE_NAME = "fds-commons-model";

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

  /**
   * singleton constructor
   */
  private ModelResource()
  {
    super();
  }
}

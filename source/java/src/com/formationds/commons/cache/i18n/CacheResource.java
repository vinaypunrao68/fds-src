/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.cache.i18n;

import com.formationds.commons.util.resource.ResourceBase;

import java.util.ResourceBundle;

/**
 * @author ptinius
 */
public class CacheResource {
  private static final String BUNDLE_NAME = "fds-commons-cache";

  private static final ResourceBundle BUNDLE =
    ResourceBundle.getBundle( BUNDLE_NAME );

  /**
   * @param key the resource key
   *
   * @return the string associated with the given resource key.
   */
  public static String getString( final String key ) {
    return ResourceBase.getString( BUNDLE, key );
  }

  /**
   * @param key the resource key
   * @param def the default value if the key isn't located
   *
   * @return the string associated with the given resource key.
   */
  public static String getString( final String key, final String def ) {
    return ResourceBase.getString( BUNDLE, key, def );
  }

  /**
   * Instantiate {@code CacheResource} object
   */
  private CacheResource() {
    super();
  }
}

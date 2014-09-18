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

package com.formationds.commons.util.resource;

import com.formationds.commons.util.i18n.CommonsUtilResource;

import java.io.File;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.text.MessageFormat;
import java.util.Enumeration;
import java.util.List;
import java.util.MissingResourceException;
import java.util.ResourceBundle;

/**
 * @author ptinius
 */
public class ResourceBase
{
  private static final String BUNDLE_NOT_FOUND =
    CommonsUtilResource.getString( "specified.bundle.not.found" );

  /**
   * @param bundle the {@link java.util.ResourceBundle}
   * @param key the resource key
   *
   * @return the string associated with the given resource key.
   */
  public static String getString( final ResourceBundle bundle,
                                  final String key )
  {
    try
    {
      return bundle.getString( key );
    }
    catch( MissingResourceException e )
    {
      return '!' + key + '!';
    }
  }

  /**
   * @param bundle the {@link java.util.ResourceBundle}
   * @param key the resource key
   * @param def the default
   *
   * @return the string associated with the given resource key.
   */
  public static String getString( final ResourceBundle bundle,
                                  final String key,
                                  final String def )
  {
    try
    {
      return bundle.getString( key );
    }
    catch( MissingResourceException e )
    {
      return def;
    }
  }

  /**
   * @param bundle the {@link java.util.ResourceBundle}
   * @param key the resource key
   * @param def the default value if the key does not exists within the bundle
   *
   * @return the string associated with the given resource key.
   */
  @SuppressWarnings( "UnusedDeclaration" )
  public static int getInt( final ResourceBundle bundle,
                            final String key,
                            final int def )
  {
    try
    {
      return Integer.valueOf( getString( bundle, key ) );
    }
    catch( Exception e )
    {
      return def;
    }
  }

  /**
   * @param paths a {@link java.util.List} of {@link String} paths to search for
   * bundle in
   * @param bundleName The bundle name to search for
   *
   * @return Returns {@link java.util.ResourceBundle}
   *
   * @throws ResourceException if the resource bundle cannot be found
   */
  @SuppressWarnings( { "unused" } )
  public static ResourceBundle find( final List<String> paths,
                                     final String bundleName )
    throws ResourceException
  {
    for( String path : paths )
    {
      final String bundle = path + File.separator + bundleName;
      if( Files.exists( Paths.get( bundle ) ) )
      {
        return ResourceBundle.getBundle( bundle );
      }
    }

    throw new ResourceException(
      MessageFormat.format( BUNDLE_NOT_FOUND,
                            bundleName )
    );
  }

  /**
   * @return an <code>Enumeration</code> of the keys contained in
   * this <code>ResourceBundle</code> and its parent bundles.
   */
  @SuppressWarnings( "UnusedDeclaration" )
  public Enumeration<String> getKeys( final ResourceBundle bundle )
  {
    return bundle.getKeys();
  }

  /**
   *
   */
  public ResourceBase()
  {
    super();
  }
}

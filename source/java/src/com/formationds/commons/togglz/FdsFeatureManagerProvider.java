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

package com.formationds.commons.togglz;

import com.formationds.commons.togglz.feature.flag.FdsFeatureToggles;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.togglz.core.manager.FeatureManager;
import org.togglz.core.manager.FeatureManagerBuilder;
import org.togglz.core.repository.file.FileBasedStateRepository;
import org.togglz.core.user.NoOpUserProvider;
import org.togglz.core.user.UserProvider;

import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;

/**
 * @author ptinius
 */
public class FdsFeatureManagerProvider
{
  private static final Logger logger =
    LoggerFactory.getLogger( FdsFeatureManagerProvider.class );

  private static final String DIRNAME_PRODUCTION = "/fds/etc/";
  private static final String DIRNAME_DEV =
    "/Volumes/LaCie/Sandbox/playground/src/main/config/";

  private static final String BASENAME_PRODUCTION = "fds-features.conf";

  private static boolean LOG_ONCE = false;
  private static String MATCHED_PATH = null;

  private static FileBasedStateRepository stateRepository = null;
  private static NoOpUserProvider userProvider = null;

  private static final List<String> BUNDLE_PATHS = new ArrayList<String>();
  static
  {
    // default production location
    BUNDLE_PATHS.add( DIRNAME_PRODUCTION );
    // development location
    BUNDLE_PATHS.add( DIRNAME_DEV );
  }

  private static FeatureManager instance = null;

  /**
   * singleton instance of FdsFeatureManagerProvider
   */
  public static FeatureManager getFeatureManager()
  {
    if( instance == null )
    {
      instance =
        new FeatureManagerBuilder()
          .featureEnum( FdsFeatureToggles.class )
          .stateRepository( getStateRepository() )
          .userProvider( getUserProvider() )
          .build();
    }

    return instance;
  }

  /**
   * @return Returns the state repository file name.
   */
  private static FileBasedStateRepository getStateRepository()
  {
    if( stateRepository == null )
    {
      for ( String b : BUNDLE_PATHS )
      {
        String feature_file = b + BASENAME_PRODUCTION;
        if( !LOG_ONCE )
        {
          LOG_ONCE = true;
          logger.debug( "looking feature file: " + feature_file );
        }

        if( Files.exists( Paths.get( feature_file ) ) )
        {
          logger.debug( "feature file match found: " + feature_file );
          stateRepository = new FileBasedStateRepository( Paths.get( feature_file )
                                                               .toFile() );
          break;
        }
      }
    }

    return stateRepository;
  }

  /**
   * @return Returns the enabled {@link org.togglz.core.user.UserProvider}
   */
  private static UserProvider getUserProvider()
  {
    if( userProvider == null )
    {
      userProvider = new NoOpUserProvider();
    }

    return userProvider;
  }

  /**
   * singleton constructor
   */
  private FdsFeatureManagerProvider()
  {
    super();
  }
}

/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.togglz;

import com.formationds.commons.togglz.feature.flag.FdsFeatureToggles;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.togglz.core.Feature;
import org.togglz.core.manager.TogglzConfig;
import org.togglz.core.repository.StateRepository;
import org.togglz.core.repository.file.FileBasedStateRepository;
import org.togglz.core.user.NoOpUserProvider;
import org.togglz.core.user.UserProvider;

import java.io.File;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;

/**
 * @author ptinius
 */
public class FdsTogglzConfig
    implements TogglzConfig {

    private static final Logger logger =
        LoggerFactory.getLogger( FdsTogglzConfig.class );

    private static final String DEF_SINGLE_NODE = "/fds/etc";

    private static final String BASENAME_PRODUCTION = "fds-features.conf";

    private static boolean LOG_ONCE = false;

    private static final List<String> BUNDLE_PATHS = new ArrayList<>();

    static {

        // better be set, otherwise we will have issues
        if( System.getProperty( "fds-root" ) != null ) {

            BUNDLE_PATHS.add( Paths.get( System.getProperty( "fds-root" ),
                                         "etc" )
                                   .toString() );
        }

        // default single node production location
        BUNDLE_PATHS.add( DEF_SINGLE_NODE );
    }

    private FileBasedStateRepository repository = null;
    private UserProvider userProvider = null;

    /**
     * Used to tell Togglz about the feature enum that you want to use. Please
     * note that your feature enum has to implement the {@link
     * org.togglz.core.Feature} interface.
     *
     * @return The feature enum, never <code>null</code>
     */
    @Override
    public Class<? extends Feature> getFeatureClass( ) {

        return FdsFeatureToggles.class;
    }

    /**
     * <p> The {@link org.togglz.core.repository.StateRepository} Togglz should
     * use to store feature state. Please refer to the Togglz documentation of
     * a list of default implementations that ship with Togglz. </p> <p> <p>
     * Please note that this method is only called once. So you can safely
     * implement the method by returning a new instance of an anonymous class.
     * </p>
     *
     * @return The repository, never <code>null</code>
     */
    @Override
    public StateRepository getStateRepository( ) {

        if( repository == null ) {

            for( String b : BUNDLE_PATHS ) {

                String feature_file = b + File.separator + BASENAME_PRODUCTION;
                if( !LOG_ONCE ) {

                    LOG_ONCE = true;
                    logger.debug( "looking feature file: " + feature_file );
                }

                if( Files.exists( Paths.get( feature_file ) ) ) {

                    logger.debug(
                        "feature file match found: " + feature_file );

                    repository =
                        new FileBasedStateRepository( Paths.get( feature_file )
                                                           .toFile() );
                    break;
                }
            }
        }

        return repository;
    }

    /**
     * <p> The {@link org.togglz.core.user.UserProvider} Togglz should use to
     * obtain the current user. Please refer to the Togglz documentation of a
     * list of default implementations that ship with Togglz. If you don't want
     * to be able to toggle feature on a per user basis and are not planning to
     * use the Togglz Console, you can return {@link NoOpUserProvider} here.
     * </p> <p> <p> Please note that this method is only called once. So you
     * can safely implement the method by returning a new instance of an
     * anonymous class. </p>
     *
     * @return The feature user provider, never <code>null</code>
     */
    @Override
    public UserProvider getUserProvider( ) {

        if( userProvider == null ) {

            userProvider = new NoOpUserProvider();
        }

        return userProvider;
    }
}

/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.togglz;

import com.formationds.commons.libconfig.ParsedConfig;
import com.formationds.commons.togglz.feature.flag.FdsFeatureToggles;
import com.formationds.commons.togglz.repository.fds.PlatformDotConfStateRepository;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.togglz.core.Feature;
import org.togglz.core.manager.TogglzConfig;
import org.togglz.core.repository.StateRepository;
import org.togglz.core.user.NoOpUserProvider;
import org.togglz.core.user.UserProvider;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.text.ParseException;

/**
 * @author ptinius
 */
public class FdsTogglzConfig
    implements TogglzConfig {

    private static final Logger logger =
        LoggerFactory.getLogger( FdsTogglzConfig.class );

    private static final String FAILED_LOAD_FEATURE_TOGGLES =
        "Failed to load feature toggles ( %s ), " +
        "Toggles will be disabled by default.";

    private static final String FAILED_PARSE_FEATURE_TOGGLES =
        "Failed to parse feature toggles ( %s ), " +
        "Toggles will be disabled by default.";

    private static final String DEF_FDS = "fds";
    private static final String DEF_ETC = "etc";

    private static final String PLATFORM_DOT_CONF = "platform.conf";

    private PlatformDotConfStateRepository repository = null;
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
            Path featureFile;

            if( System.getProperty( "fds-root" ) != null ) {

                featureFile = Paths.get( System.getProperty( "fds-root" ),
                                         DEF_ETC,
                                         PLATFORM_DOT_CONF );
            } else {

                featureFile = Paths.get( DEF_FDS,
                                         DEF_ETC,
                                         PLATFORM_DOT_CONF );
            }

            logger.debug( "looking feature file: " + featureFile );

            if( Files.exists( featureFile ) ) {

                logger.debug(
                    "feature file match found: " + featureFile );
                try
                {
                    repository =
                        new PlatformDotConfStateRepository(
                            new ParsedConfig(
                                Files.newInputStream( featureFile ) ) );
                }
                catch( IOException e )
                {
                    logger.error(
                        String.format( FAILED_LOAD_FEATURE_TOGGLES,
                                       featureFile ),
                        e );
                }
                catch( ParseException e )
                {
                    logger.error(
                        String.format( FAILED_PARSE_FEATURE_TOGGLES,
                                       featureFile ),
                        e );
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

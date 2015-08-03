/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.togglz.repository.fds;

import com.formationds.commons.libconfig.ParsedConfig;
import com.formationds.commons.togglz.feature.flag.FdsFeatureToggles;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.togglz.core.Feature;
import org.togglz.core.repository.FeatureState;
import org.togglz.core.repository.StateRepository;

/**
 * @author ptinius
 */
public class PlatformDotConfStateRepository
    implements StateRepository
{
    private static final Logger logger =
        LoggerFactory.getLogger( PlatformDotConfStateRepository.class );

    private final ParsedConfig repo;

    /**
     * @param repo the {@link ParsedConfig}
     */
    public PlatformDotConfStateRepository( final ParsedConfig repo )
    {
        this.repo = repo;
    }

    /**
     * Get the persisted state of a feature from the repository. If the
     * repository doesn't contain any information regarding this feature
     * it must return <code>null</code>.
     *
     * @param feature The feature to read the state for
     * @return The persisted feature state or <code>null</code>
     */
    @Override
    public FeatureState getFeatureState( final Feature feature )
    {
        FeatureState state = null;

        logger.trace( "feature looking up {} ({}).",
                      getFeatureName( feature ),
                      getFDSFeatureName( feature ) );

        final boolean flag =
            repo.defaultBoolean( getFDSFeatureName( feature ), false );

        logger.trace( "found feature {} ({}) with a setting of \"{}\".",
                      getFeatureName( feature ),
                      getFDSFeatureName( feature ),
                      flag );

        state = new FeatureState( feature );
        state.setEnabled( flag );

        /**
         * In the future we may want to extend our feature togglz to
         * include strategy. But for now default no strategy
         */
        state.setStrategyId( null );

        /**
         * In the future we may want to extend our feature togglz to
         * include parameters. But for now no parameters are available.
         */

        /**
         * In the future we may want to extend our feature togglz to
         * include users. But for now no parameters are available.
         */

        return state;
    }

    /**
     * Persist the supplied feature state. The repository implementation must ensure that subsequent calls to
     * {@link #getFeatureState(Feature)} return the same state as persisted using this method.
     *
     * @param featureState The feature state to persist
     * @throws UnsupportedOperationException if this state repository does not support updates
     */
    @Override
    public void setFeatureState( final FeatureState featureState )
    {
        throw new UnsupportedOperationException(
            "setFeatureState is not unsupported" );
    }

    private String getFeatureName( final Feature feature )
    {
        return feature.name();
    }

    private String getFDSFeatureName( final Feature feature )
    {
        return ( ( FdsFeatureToggles ) feature ).fdsname( );
    }

    private boolean isTrue( final String s )
    {
        return s != null &&
               ( "true".equalsIgnoreCase( s.trim( ).toLowerCase() ) ||
                 "yes".equalsIgnoreCase( s.trim( ).toLowerCase( ) ) ||
                 "enabled".equalsIgnoreCase( s.trim( ).toLowerCase( ) ) ||
                 "enable".equalsIgnoreCase( s.trim( ).toLowerCase() ) );
    }
}

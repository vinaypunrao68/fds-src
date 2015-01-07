/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.snmp;

import com.formationds.commons.model.entity.Event;
import com.formationds.commons.togglz.feature.flag.FdsFeatureToggles;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.io.Serializable;
import java.nio.file.Paths;

/**
 * @author ptinius
 */
public class SnmpManager
    implements Serializable {

    private static final long serialVersionUID = 2277997569296699748L;

    private static final Logger logger =
        LoggerFactory.getLogger( SnmpManager.class );

    /**
     * private constructor
     */
    private SnmpManager( ) {
    }

    private static class SnmpManagerHolder {

        protected static final SnmpManager INSTANCE = new SnmpManager();

    }

    /**
     * @return Returns SnmpManager instance
     */
    public static SnmpManager instance( ) {

        return SnmpManagerHolder.INSTANCE;

    }

    /**
     * @return Returns the {@link SnmpManager
     */
    protected Object readResolve( ) {
        return instance();
    }

    private TrapSend trap = null;

    private enum DisableReason {

        INITIALIZE_ERROR,
        MISSING_CONFIG,
        SEND_ERROR,
    }

    private void initialize() {

            if( trap == null ) {

                logger.info( "SNMP feature is enabled. Initialize SNMP components." );

                if( System.getProperty( TrapSend.SNMP_TARGET_KEY ) == null ) {

                    disable( DisableReason.MISSING_CONFIG );

                } else {

                    try {

                        this.trap = new TrapSend();

                    } catch( IOException e ) {

                        disable( DisableReason.INITIALIZE_ERROR );

                    }

                }
            }
    }

    private void disable( final DisableReason reason ) {

        if( FdsFeatureToggles.SNMP.isActive() ) {

            FdsFeatureToggles.SNMP.state( false );
            switch( reason ) {

                case INITIALIZE_ERROR:

                    logger.error(
                        "Disabled SNMP feature due initialization error." );
                    break;

                case MISSING_CONFIG:

                    logger.warn(
                        Paths.get( System.getProperty( "fds-root" ),
                                   "etc",
                                   "platform.conf" )
                             .toString() +
                            " is missing the snmp required configuration keys." );
                    logger.error(
                        "Disabled SNMP feature due to configuration error." );
                    break;

                case SEND_ERROR:

                    logger.error(
                        "Disabled SNMP feature due sending error." );
                    break;
            }

            logger.trace( FdsFeatureToggles.SNMP.name() + " is active? " +
                              FdsFeatureToggles.SNMP.isActive() );
        }
    }

    public void notify( final Event event ) {

        if( FdsFeatureToggles.SNMP.isActive() ) {

            initialize();

            if( FdsFeatureToggles.SNMP.isActive() ) {

                try {

                    this.trap.notify( event );

                } catch( IOException e ) {
                    e.printStackTrace();
                }
            }

        }
    }
}

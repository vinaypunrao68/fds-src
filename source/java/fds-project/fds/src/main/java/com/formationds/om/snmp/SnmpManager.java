/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.snmp;

import com.formationds.commons.model.entity.Event;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.io.Serializable;
import java.nio.file.Paths;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * @author ptinius
 */
public class SnmpManager
    implements Serializable {

    private static final long serialVersionUID = 2277997569296699748L;

    private static final Logger logger =
        LoggerFactory.getLogger( SnmpManager.class );

    private AtomicBoolean ACTIVE;

    /**
     * private constructor
     */
    private SnmpManager( ) {
        this.ACTIVE = new AtomicBoolean( true );
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
    private boolean logOnce = false;

    public enum DisableReason {

        INITIALIZE_ERROR,
        MISSING_CONFIG,
        SEND_ERROR,
    }

    private void initialize() {

        if( trap == null ) {

            if( !ACTIVE.get() && !logOnce ) {

                logger.info( "SNMP feature is disabled. " +
                             "No Initialization of SNMP components." );
                logOnce = true;

                return;
            }

            if( System.getProperty( TrapSend.SNMP_TARGET_KEY ) == null ) {

                disable( DisableReason.MISSING_CONFIG );

            } else {

                try {
                    logger.info( "SNMP feature is enabled. Initialize SNMP components." );
                    this.trap = new TrapSend();

                } catch( IOException e ) {

                    disable( DisableReason.INITIALIZE_ERROR );

                }

            }
        }
    }

    public void disable( final DisableReason reason ) {

        if( ACTIVE.compareAndSet( true, false ) ) {

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

            logger.trace( "SNMP is active? " + ACTIVE.toString() );
        }
    }

    public void notify( final Event event ) {

        if( ACTIVE.get() ) {

            initialize();

            /*
             * the above call may disable SNMP if it cannot
             * successfully initialize.
             */
            if( ACTIVE.get() ) {

                try {

                    this.trap.notify( event );

                } catch( Exception e ) {

                    logger.error( e.getMessage(), e );
                }
            }

        }
    }
}

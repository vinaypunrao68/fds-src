/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.events;

/**
 * TODO: should we just use existing Logger level/priorities?
 */
public enum EventSeverity {

    /** The event is informational in nature.  No administrator action is required */
    INFO,

    /** Represents a configuration change */
    CONFIG,

    /**
     * The event represents a warning that something in the system has occurred that
     * may require administrator action to identify and resolve.  The system is operating
     * normally at this time.
     */
    WARNING,

    /**
     * An error has occurred in the system and may require administrator action to
     * identify and resolve.  One or more features may not be operating properly, but
     * the system as a whole is operating.
     */
    ERROR,

    /**
     * A critical event indicates that one or more subsystems has an error that requires
     * immediate administrator action to identify and resolve.
     */
    CRITICAL,

    /**
     * A fatal event indicates that one or more subsystems has failed and the system is
     * not operating properly.  Recovery attempts have failed and immediate administrator
     * action is required.
     */
    FATAL,

    /** Unknown severity */
    UNKNOWN
}

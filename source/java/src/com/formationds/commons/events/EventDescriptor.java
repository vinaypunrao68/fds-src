/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.events;

import java.util.List;

/**
 * Describes an event.  This is used in defining event metadata.  For a set of events, you can
 * define an enum that implements this interface to provide all the metadata.
 *
 * @see {@link com.formationds.xdi.ConfigurationApi} for an example usage.
 *
 */
public interface EventDescriptor {
    /**
     * @return the event type
     */
    public EventType type();

    /**
     *
     * @return the event category
     */
    public EventCategory category();

    /**
     *
     * @return the event severity
     */
    public EventSeverity severity();

    /**
     *
     * @return the default message
     */
    public String defaultMessage();

    /**
     *
     * @return the message argument names
      */
    public List<String> argNames();

    /**
     *
     * @return the message key
     */
    public String key();
}

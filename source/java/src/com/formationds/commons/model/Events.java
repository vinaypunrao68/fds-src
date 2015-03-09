/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.entity.Event;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 *
 */
public class Events extends ArrayList<Event> {

    /**
     *
     */
    public Events() {}

    /**
     * @param events
     */
    public Events(List<? extends Event> events) {
        super( events );
    }

    /**
     *
     * @param events
     */
    public Events(Event... events) {
        if (events != null)
        {
            super.addAll( Arrays.asList( events ) );
        }
    }

    /**
     * Set the events.
     * @param events
     */
    public Events setEvents(List<? extends Event> events)
    {
        super.clear();
        super.addAll(events);
        return this;
    }

    /**
     *
     * @return the set of events
     */
    public List<? super Event> getEvents() { return this; };

    /**
     * @param e
     * @return this
     */
    public Events addEvents(Event... e)
    {
        return (e != null ? this.addEvents(Arrays.asList(e)) : this);
    }

    /**
     *
     * @param events
     * @return this
     */
    public Events addEvents(List<? extends Event> events)
    {
        super.addAll(events);
        return this;
    }
}

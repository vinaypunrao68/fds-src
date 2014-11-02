/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.crud.SearchResults;
import com.formationds.commons.model.abs.ModelBase;
import com.formationds.commons.model.entity.Event;

import java.util.Arrays;
import java.util.LinkedList;
import java.util.List;

/**
 *
 */
public class Events extends ModelBase implements SearchResults {

    final List<? super Event> events = new LinkedList<>();

    /**
     *
     */
    public Events() {}

    /**
     * @param events
     */
    public Events(List<? extends Event> events) {
        this.addEvents(events);
    }

    /**
     *
     * @param events
     */
    public Events(Event... events) {
        this.addEvents(events);
    }

    /**
     * Set the events.
     * @param events
     */
    public Events setEvents(List<Event> events) { this.events.clear(); this.events.addAll(events); return this; }

    /**
     *
     * @return the set of events
     */
    public List<? super Event> getEvents() { return this.events; };

    /**
     * @param e
     * @return this
     */
    public Events addEvents(Event... e) { return (e != null ? this.addEvents(Arrays.asList(e)) : this); }

    /**
     *
     * @param events
     * @return this
     */
    public Events addEvents(List<? extends Event> events) { this.events.addAll(events); return this; }
}

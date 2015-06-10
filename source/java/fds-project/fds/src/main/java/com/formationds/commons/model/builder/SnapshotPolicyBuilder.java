/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.builder;

import com.formationds.commons.model.RecurrenceRule;
import com.formationds.commons.model.SnapshotPolicy;

/**
 * @author ptinius
 */
public class SnapshotPolicyBuilder {
    private long id;
    private String name;
    private String recurrenceRule;
    private long retention;                     // time in seconds

    /**
     * default constructor
     */
    public SnapshotPolicyBuilder() {
    }

    /**
     * @param id the {@code int} representing the snapshot policy id
     *
     * @return Returns {@link com.formationds.commons.model.builder.SnapshotBuilder}
     */
    public SnapshotPolicyBuilder withId( final long id ) {
        this.id = id;
        return this;
    }

    /**
     * @param name the {@link String} representing the snapshot policies name
     *
     * @return Returns {@link com.formationds.commons.model.builder.SnapshotBuilder}
     */
    public SnapshotPolicyBuilder withName( final String name ) {
        this.name = name;
        return this;
    }

    /**
     * @param recurrenceRule the {@link RecurrenceRule} representing the
     *                       snapshot policy id
     *
     * @return Returns {@link com.formationds.commons.model.builder.SnapshotBuilder}
     */
    public SnapshotPolicyBuilder withRecurrenceRule( final String recurrenceRule ) {
        this.recurrenceRule = recurrenceRule;
        return this;
    }

    /**
     * @param retention the {@code long} representing the snapshot policy
     *                  retention
     *
     * @return Returns {@link com.formationds.commons.model.builder.SnapshotBuilder}
     */
    public SnapshotPolicyBuilder withRetention( final long retention ) {
        this.retention = retention;
        return this;
    }

    /**
     * @return Returns {@link SnapshotPolicy}
     */
    public SnapshotPolicy build() {
        SnapshotPolicy snapshotPolicy = new SnapshotPolicy();
        snapshotPolicy.setId( id );
        snapshotPolicy.setName( name );
        try {

            snapshotPolicy.setRecurrenceRule( recurrenceRule );
        } catch( Exception e ) {
            e.printStackTrace();
        }

        snapshotPolicy.setRetention( retention );
        return snapshotPolicy;
    }
}

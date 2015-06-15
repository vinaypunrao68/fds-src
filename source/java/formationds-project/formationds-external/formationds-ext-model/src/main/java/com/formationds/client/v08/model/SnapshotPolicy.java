/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

import com.formationds.client.ical.RecurrenceRule;

import java.time.Duration;
import java.util.Objects;

/**
 *
 */
public class SnapshotPolicy extends AbstractResource<Long> {

    public enum SnapshotPolicyType { USER, SYSTEM_TIMELINE }

    private SnapshotPolicyType type;
    private Duration       retentionTime;
    private RecurrenceRule recurrenceRule;

    SnapshotPolicy() {}

    /**
     * @param type the policy type
     * @param recurrenceRule the recurrence rule
     * @param retentionTime the retention time
     */
    public SnapshotPolicy( SnapshotPolicyType type,
                           RecurrenceRule recurrenceRule,
                           Duration retentionTime ) {
        this( null, null, type, recurrenceRule, retentionTime );
    }

    /**
     *
     * @param policyName the policy name
     * @param type the type
     * @param recurrenceRule the recurrence rule
     * @param retentionTime the retention time
     */
    public SnapshotPolicy( String policyName,
                           SnapshotPolicyType type,
                           RecurrenceRule recurrenceRule,
                           Duration retentionTime ) {
        this( null, policyName, type, recurrenceRule, retentionTime );
    }

    /**
     *
     * @param id the system-generated id
     * @param policyName the policy name
     * @param type the type
     * @param recurrenceRule the recurrence rule
     * @param retentionTime the retention time
     */
    public SnapshotPolicy( Long id,
                           String policyName,
                           SnapshotPolicyType type,
                           RecurrenceRule recurrenceRule,
                           Duration retentionTime ) {
        super(id, policyName);
        this.type = type;
        this.retentionTime = retentionTime;
        this.recurrenceRule = recurrenceRule;
    }

    /**
     *
     * @return a new policy with the same type, recurrence rule, and retention time suitable for creating a new policy
     * based on this one.  The name and ID in the copy are null.
     */
    public SnapshotPolicy newPolicyFrom() {
        return new SnapshotPolicy( type, recurrenceRule, retentionTime );
    }

    /**
     * @return the policy type
     */
    public SnapshotPolicyType getType() { return type; }

    /**
     *
     * @param t the policy type
     */
    public void setType(SnapshotPolicyType t) { this.type = t; }

    /**
     *
     * @return the retention time
     */
    public Duration getRetentionTime() {
        return retentionTime;
    }

    /**
     *
     * @param retentionTime the retention time
     * @return this
     */
    public SnapshotPolicy withRetentionTime( Duration retentionTime ) {
        this.retentionTime = retentionTime;
        return this;
    }

    /**
     *
     * @return the recurrence rule (schedule)
     */
    public RecurrenceRule getRecurrenceRule() {
        return recurrenceRule;
    }

    /**
     *
     * @param recurrenceRule the recurrence rule (schedule)
     * @return this
     */
    public SnapshotPolicy withRecurrenceRule( RecurrenceRule recurrenceRule ) {
        this.recurrenceRule = recurrenceRule;
        return this;
    }

    @Override
    public boolean equals( Object o ) {
        if ( this == o ) { return true; }
        if ( !(o instanceof SnapshotPolicy) ) { return false; }
        if ( !super.equals( o ) ) { return false; }
        final SnapshotPolicy that = (SnapshotPolicy) o;
        return Objects.equals( type, that.type ) &&
               Objects.equals( retentionTime, that.retentionTime ) &&
               Objects.equals( recurrenceRule, that.recurrenceRule );
    }

    @Override
    public int hashCode() {
        return Objects.hash( super.hashCode(), type, retentionTime, recurrenceRule );
    }
}

/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

import java.util.Objects;

abstract class QosPolicyBase {

    // id generated when the policy preset is saved.  null indicates an unsaved preset policy
    private final Long id;

    private final int priority;
    private final int iopsMin;
    private final int iopsMax;

    /**
     *
     * @param id the id of the qos policy preset
     * @param priority the priority
     * @param iopsMin the minimum iops setting
     * @param iopsMax the maximum iops setting
     */
    QosPolicyBase( Long id, int priority, int iopsMin, int iopsMax ) {
        this.id = id;
        this.priority = priority;
        this.iopsMin = iopsMin;
        this.iopsMax = iopsMax;
    }

    /**
     *
     * @param priority the priority
     * @param iopsMin the minimum iops setting
     * @param iopsMax the maximum iops setting
     */
    public QosPolicyBase( int priority, int iopsMin, int iopsMax ) {
        this.id = null;
        this.priority = priority;
        this.iopsMin = iopsMin;
        this.iopsMax = iopsMax;
    }

    public Long getId() { return id; }
    public int getPriority() {  return priority; }
    public int getIopsMin() { return iopsMin; }
    public int getIopsMax() { return iopsMax; }

    /**
     *
     * @param id the id for the qos policy preset
     * @return a new instance with the specified id and the settings for this policy
     */
    QosPolicyPreset withId(Long id) {
        if (this.id != null) {
            throw new IllegalStateException( "Only generate id for presets that do not yet have an id");
        }
        return new QosPolicyPreset( id, this.priority, this.iopsMin, this.iopsMax );
    }

    public boolean idEquals( Object o ) {
        if ( this == o ) { return true; }
        if ( !(o instanceof QosPolicyBase) ) { return false; }
        final QosPolicyBase that = (QosPolicyBase) o;
        return Objects.equals( id, that.id );

    }

    @Override
    public boolean equals( Object o ) {
        if ( this == o ) { return true; }
        if ( !(o instanceof QosPolicyBase) ) { return false; }
        final QosPolicyBase that = (QosPolicyBase) o;
        return Objects.equals( priority, that.priority ) &&
               Objects.equals( iopsMin, that.iopsMin ) &&
               Objects.equals( iopsMax, that.iopsMax );
    }

    @Override
    public int hashCode() {
        return Objects.hash( priority, iopsMin, iopsMax );
    }

    @Override
    public String toString() {
        final StringBuilder sb = new StringBuilder( "QosPolicyBase{" );
        sb.append( "id=" ).append( id );
        sb.append( ", priority=" ).append( priority );
        sb.append( ", iopsMin=" ).append( iopsMin );
        sb.append( ", iopsMax=" ).append( iopsMax );
        sb.append( '}' );
        return sb.toString();
    }
}

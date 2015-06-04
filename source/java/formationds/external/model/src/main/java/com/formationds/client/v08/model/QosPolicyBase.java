/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

import java.util.Objects;

abstract class QosPolicyBase {

    private int priority;
    private int iopsMin;
    private int iopsMax;

    protected QosPolicyBase() {}

    /**
     *
     * @param priority the priority
     * @param iopsMin the minimum iops setting
     * @param iopsMax the maximum iops setting
     */
    public QosPolicyBase( int priority, int iopsMin, int iopsMax ) {
        this.priority = priority;
        this.iopsMin = iopsMin;
        this.iopsMax = iopsMax;
    }

    public int getPriority() {  return priority; }
    public int getIopsMin() { return iopsMin; }
    public int getIopsMax() { return iopsMax; }

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
        sb.append( ", priority=" ).append( priority );
        sb.append( ", iopsMin=" ).append( iopsMin );
        sb.append( ", iopsMax=" ).append( iopsMax );
        sb.append( '}' );
        return sb.toString();
    }
}

/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

import java.util.Objects;

public class QosPolicy extends QosPolicyBase {

    public static QosPolicy of(int priority, int iopsMin, int iopsMax ) {
        return new QosPolicy( priority, iopsMin, iopsMax );
    }
    public static QosPolicy fromPreset(QosPolicyPreset p) {
        return new QosPolicy( p.getPriority(), p.getIopsMin(), p.getIopsMax() );
    }

    private Long presetID;

    protected QosPolicy() {}
    public QosPolicy( int priority, int iopsMin, int iopsMax ) {
        super(priority, iopsMin, iopsMax );
    }

    public QosPolicy( Long presetID, int priority, int iopsMin, int iopsMax ) {
        super(priority, iopsMin, iopsMax );
        this.presetID = presetID;
    }

    public Long getPresetID() {
        return presetID;
    }

    public void setPresetID( Long presetID ) {
        this.presetID = presetID;
    }

    @Override
    public boolean equals( Object o ) {
        if ( this == o ) { return true; }
        if ( !(o instanceof QosPolicy) ) { return false; }
        if ( !super.equals( o ) ) { return false; }
        final QosPolicy qosPolicy = (QosPolicy) o;
        return Objects.equals( presetID, qosPolicy.presetID );
    }

    @Override
    public int hashCode() {
        return Objects.hash( super.hashCode(), presetID );
    }

}
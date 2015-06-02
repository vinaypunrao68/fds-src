/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;


public class QosPolicy extends QosPolicyBase {

    public static QosPolicy fromPreset(QosPolicyPreset p) {
        return new QosPolicy( p.getId(), p.getPriority(), p.getIopsMin(), p.getIopsMax() );
    }

    private Long presetID;

    public QosPolicy( int priority, int iopsMin, int iopsMax ) {
        super(priority, iopsMin, iopsMax );
    }

    public QosPolicy( Long presetID, int priority, int iopsMin, int iopsMax ) {
        super(priority, iopsMin, iopsMax );
        this.presetID = presetID;
    }

    public QosPolicy( Long id, Long presetID, int priority, int iopsMin, int iopsMax ) {
        super(id, priority, iopsMin, iopsMax );
        this.presetID = presetID;
    }

    public Long getPresetID() {
        return presetID;
    }

    public void setPresetID( Long presetID ) {
        this.presetID = presetID;
    }
}

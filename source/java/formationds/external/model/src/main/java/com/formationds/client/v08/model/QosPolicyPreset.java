/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

public class QosPolicyPreset extends QosPolicyBase {
    /**
     *
     * @param id the id of the qos policy preset
     * @param priority the priority
     * @param iopsMin the minimum iops setting
     * @param iopsMax the maximum iops setting
     */
    QosPolicyPreset( Long id, int priority, int iopsMin, int iopsMax ) {
        super(id, priority, iopsMin, iopsMax );
    }

    /**
     *
     * @param priority the priority
     * @param iopsMin the minimum iops setting
     * @param iopsMax the maximum iops setting
     */
    public QosPolicyPreset( int priority, int iopsMin, int iopsMax ) {
        super(priority, iopsMin, iopsMax );
    }

}

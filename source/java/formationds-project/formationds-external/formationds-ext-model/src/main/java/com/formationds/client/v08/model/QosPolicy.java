/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

import java.util.Objects;

public class QosPolicy extends QosPolicyBase {

    /**
     *
     * @return the default policy: priority=7; min iops=0 (no guarantee); max iops=0 (unlimited)
     */
    public static QosPolicy defaultPolicy() {
        return new QosPolicy(7, 0, 0);
    }

    /*
     * from cli doc...
     * __createParser.add_argument( self.arg_str + AbstractPlugin.iops_limit_str,
     *      help="The IOPs limit for the volume.  0 = unlimited and is the default if not specified.",
     *      type=VolumeValidator.iops_limit, default=0, metavar="" )
     *  __createParser.add_argument( self.arg_str + AbstractPlugin.iops_guarantee_str,
     *      help="The IOPs guarantee for this volume.  0 = no guarantee and is the default if not
     *          specified.",
     *      type=VolumeValidator.iops_guarantee, default=0, metavar="" )
     *  __createParser.add_argument( self.arg_str + AbstractPlugin.priority_str,
     *      help="A value that indicates how to prioritize performance for this volume.  1 = highest
     *          priority, 10 = lowest.  Default value is 7.",
     *      type=VolumeValidator.priority, default=7, metavar="")
     *
     */
    /**
     * Create a QosPolicy
     *
     * @param priority the qos priority
     * @param iopsMin the minimum iops
     * @param iopsMax the max iops
     *
     * @return the new QosPolicy
     */
    public static QosPolicy of(int priority, int iopsMin, int iopsMax ) {
        return new QosPolicy( priority, iopsMin, iopsMax );
    }

    /**
     * Create a new QosPolicy from the specified preset
     *
     * @param p the preset
     *
     * @return the new QosPolicy based on the preset.
     */
    public static QosPolicy fromPreset(QosPolicyPreset p) {
        return new QosPolicy( p.getId(), p.getPriority(), p.getIopsMin(), p.getIopsMax() );
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
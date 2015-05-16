/**
 * Copyright (c) 2014 Formation Data Systems.  All rights reserved.
 */

package com.formationds.commons.model.entity;

/**
 * Extracted from VolumeDatapoint and used in base of
 * generic type parameter expressions for ? extends IVolumeDatapoint
 * in repository query results.
 */
public interface IVolumeDatapoint {

    /**
     *
     * @return the value for the volume datapoint
     */
    Double getValue();

    /**
     *
     * @return the timestamp in seconds since the epoch
     */
    Long getTimestamp();

    /**
     *
     * @param timestamp in seconds since the epoch
     */
    void setTimestamp(long timestamp);

    /**
     *
     * @return the volume name
     */
    String getVolumeName();

    /**
     *
     * @return the volume id
     */
    String getVolumeId();

    /**
     * Set the volume id
     *
     * @param anId the volume id
     */
    void setVolumeId( String anId );

    /**
     *
     * @return the metric lookup key
     */
    // TODO: convert to a Metrics enum value?
    String getKey();
}

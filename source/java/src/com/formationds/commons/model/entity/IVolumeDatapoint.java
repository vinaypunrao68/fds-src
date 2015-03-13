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
    Double getValue();

    Long getTimestamp();

    String getVolumeName();

    String getVolumeId();
    
    void setVolumeId( String anId );

    String getKey();
}

/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.type;

/**
 * @author ptinius
 */
public enum ServiceStatus {
    /*
     * should be consistent with source/include/fdsp/fds_service_types.cpp
     */
    INVALID,
    ACTIVE,
    INACTIVE,
    ERROR;

    public static ServiceStatus findBy( final int ordinal ) {

        for( final ServiceStatus status : ServiceStatus.values() ) {

            if( status.ordinal() == ordinal ) {

                return status;

            }
        }

        // unknown service state default to invalid
        return INVALID;
    }
}

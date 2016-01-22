/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.type;

import java.util.Optional;

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


    public static Optional<ServiceStatus> fromThriftServiceStatus(
        final com.formationds.protocol.svc.types.ServiceStatus status ) {

        switch( status ) {
            case SVC_STATUS_INVALID:
                return Optional.of( INVALID );
            case SVC_STATUS_ACTIVE:
                return Optional.of( ACTIVE );
            case SVC_STATUS_INACTIVE_STOPPED:
	        case SVC_STATUS_INACTIVE_FAILED:
                return Optional.of( INACTIVE );
        }

        return Optional.empty();

    }
}

/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.model;

public class Service extends AbstractResource<Long> {
    public enum ServiceStatus {
        UP,
        DOWN,
        ALL_AROUND,
        HAPPY,
        SAD,
        PANICING,
        MISERABLE;
    }

    private final ServiceType type;
    private final int port;
    private final ServiceStatus status;

    public Service(Long id, ServiceType type,  int port, ServiceStatus status) {
        super(id, type.getFullName(), Integer.toString( port ));
        this.type = type;
        this.port = port;
        this.status = status;
    }

    public ServiceType getType() {
        return type;
    }

    public int getPort() {
        return port;
    }

    public ServiceStatus getStatus() {
        return status;
    }


}

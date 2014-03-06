package com.formationds.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

public class Service {
    private String uuid;
    private String name;
    private ServiceType serviceType;
    private String ipAddress;
    private int port;
    private ServiceState serviceState;

    public Service(String uuid, String name, String serviceType, String ipAddress, int port, int serviceState) {
        this.uuid = uuid;
        this.name = name;
        this.serviceType = ServiceType.valueOf(serviceType);
        this.ipAddress = ipAddress;
        this.port = port;
        this.serviceState = ServiceState.values()[serviceState];
    }

    public String getUuid() {
        return uuid;
    }

    public String getName() {
        return name;
    }

    public String getIpAddress() {
        return ipAddress;
    }

    public int getPort() {
        return port;
    }

    public ServiceState getServiceState() {
        return serviceState;
    }

    public ServiceType getServiceType() {
        return serviceType;
    }
}

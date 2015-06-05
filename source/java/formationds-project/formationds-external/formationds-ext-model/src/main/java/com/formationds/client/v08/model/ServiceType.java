/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

public enum ServiceType {
    PM("Platform Manager", "platformd"),
    OM("Orchestration Manager", "java com.formationds.om.Main"),
    AM("Access Manager", "bare_am"),
    SM("Storage Manager", "StorMgr"),
    DM("Data Manager", "DataMgr"),
    XDI("eXtensible Data Interface", "java com.formationds.am.Main");

    private final String fullName;
    private final String processName;

    ServiceType(String n, String p) {
        this.fullName = n;
        this.processName = p;
    }

    public String getFullName() {
        return fullName;
    }

    public String getProcessName() {
        return processName;
    }
}

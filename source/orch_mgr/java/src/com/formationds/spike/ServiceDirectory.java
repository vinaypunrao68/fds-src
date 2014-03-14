package com.formationds.spike;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.FDSP_MgrIdType;
import FDS_ProtocolInterface.FDSP_RegisterNodeType;
import org.joda.time.DateTime;

import java.util.concurrent.ConcurrentHashMap;
import java.util.stream.Stream;

public class ServiceDirectory {
    private ConcurrentHashMap<FDSP_RegisterNodeType, DateTime> endpoints;

    public ServiceDirectory() {
        endpoints = new ConcurrentHashMap<>();
    }

    public void add(FDSP_RegisterNodeType nodeType) {
        endpoints.putIfAbsent(nodeType, DateTime.now());
    }

    public Stream<FDSP_RegisterNodeType> allNodes() {
        return endpoints.keySet().stream();
    }

    public Stream<FDSP_RegisterNodeType> byType(FDSP_MgrIdType nodeType) {
        return endpoints.keySet().stream().filter(n -> n.getNode_type().equals(nodeType));
    }
}

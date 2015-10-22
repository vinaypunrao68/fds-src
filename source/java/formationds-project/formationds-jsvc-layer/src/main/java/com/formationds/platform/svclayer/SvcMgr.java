/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */
package com.formationds.platform.svclayer;

import com.formationds.protocol.svc.types.FDSP_MgrIdType;

// TODO: placeholder if we want to mirror C++ design in Java.  Not absolutely sure it is necessary.
public class SvcMgr {

    /**
     * Map the service port as an offset from the specified base port (i.e. platform port)
     *
     * @param basePort the base port
     * @param serviceType the service type
     *
     * @return the service port offset from the base port
     */
    public static int mapToServicePort(int basePort, FDSP_MgrIdType serviceType) {
        int offset = serviceType.getValue() - FDSP_MgrIdType.FDSP_PLATFORM.getValue();
        return basePort + offset;
    }
}

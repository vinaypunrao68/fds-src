/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.snmp;

import org.snmp4j.smi.OID;

/**
 * @author ptinius
 */
public class FDSOid {

    // general
    private static final int ORG = 1;
    private static final int ID = 3;
    private static final int DOD = 6;
    private static final int INTERNET = 1;
    private static final int PRIVATE = 4;
    private static final int ENTERPRISE = 1;

/*
 * OID format:
 * iso(1).identified-organization(3)
 *       .dod(6)
 *       .internet(1)
 *       .private(4)
 *       .enterprise(1)
 *       .fds(50000)            // company
 *       .ds(1)                 // product
 *
 * NOTE:: None of the following Object IDs are registered.
 * TODO register object ids
 *
 * MIB              Object Identifier
 * ----------------+-------------------
 * fds	            1.3.6.1.4.1.50000
 * ds	           	1.3.6.1.4.1.50000.1
 * experimental	   	1.3.6.1.3
 * eventTime	    1.3.6.1.4.1.50000.1.1
 * eventServiceName	1.3.6.1.4.1.50000.1.2
 * snmpTrapAddress	1.3.6.1.6.3.18.1.3.0
 * eventType        1.3.6.1.4.1.50000.1.3
 * eventCategory	1.3.6.1.4.1.50000.1.4
 * eventSeverity	1.3.6.1.4.1.50000.1.5
 * eventMessage	    1.3.6.1.4.1.50000.1.5
 */

    // fds specific
    private static final int FDS = 50000;
    private static final int DS = 1;
    private static final int EVENT_TIME = 1;
    private static final int EVENT_SERVICE_NAME = 2;
    private static final int EVENT_TYPE = 3;
    private static final int EVENT_CATEGORY = 4;
    private static final int EVENT_SEVERITY = 5;
    private static final int EVENT_MESSAGE = 6;

    public static final OID FDS_OID =
        new OID( new int[] {
            ORG,
            ID,
            DOD,
            INTERNET,
            PRIVATE,
            ENTERPRISE,
            FDS } );
    public static final OID DS_OID =
        new OID( new int[] {
            ORG,
            ID,
            DOD,
            INTERNET,
            PRIVATE,
            ENTERPRISE,
            FDS,
            DS } );
    public static final OID EVENT_TIME_OID =
        new OID( new int[] {
            ORG,
            ID,
            DOD,
            INTERNET,
            PRIVATE,
            ENTERPRISE,
            FDS,
            DS,
            EVENT_TIME } );
    public static final OID EVENT_SERVICE_NAME_OID =
        new OID( new int[] {
            ORG,
            ID,
            DOD,
            INTERNET,
            PRIVATE,
            ENTERPRISE,
            FDS,
            DS,
            EVENT_SERVICE_NAME } );
    public static final OID EVENT_TYPE_OID =
        new OID( new int[] {
            ORG,
            ID,
            DOD,
            INTERNET,
            PRIVATE,
            ENTERPRISE,
            FDS,
            DS,
            EVENT_TYPE } );
    public static final OID EVENT_CATEGORY_OID =
        new OID( new int[] {
            ORG,
            ID,
            DOD,
            INTERNET,
            PRIVATE,
            ENTERPRISE,
            FDS,
            DS,
            EVENT_CATEGORY } );
    public static final OID EVENT_SEVERITY_OID =
        new OID( new int[] {
            ORG,
            ID,
            DOD,
            INTERNET,
            PRIVATE,
            ENTERPRISE,
            FDS,
            DS,
            EVENT_SEVERITY } );
    public static final OID EVENT_MESSAGE_OID =
        new OID( new int[] {
            ORG,
            ID,
            DOD,
            INTERNET,
            PRIVATE,
            ENTERPRISE,
            FDS,
            DS,
            EVENT_MESSAGE } );

}

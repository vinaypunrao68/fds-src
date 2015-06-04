/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

/**
 * The supported client feature set as activated through user roles
 */
public enum Feature {

    /** system management and admin features*/
    SYS_MGMT,

    /** tenant management and admin features*/
    TENANT_MGMT,

    /** volume management features */
    VOL_MGMT,

    /** user management features */
    USER_MGMT
}

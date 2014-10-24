/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.events;

/**
 * Audit Event Visibility Scoping.
 *
 * Global Admin has access to all events;
 * Tenant Admin has access to all events relevant to that Tenant;
 * Owner events are accessible to admins and the owner of the associated object;
 * User events are visible to the specific user of the system, along with the owner, tenant admin, and global admin.
 *
 */
public enum AuditScope {
    /**
     * Event is visible only to global admin users
     */
    GLOBAL_ADMIN,

    /**
     * Event is visible to the tenant admin and the global admin
     */
    TENANT_ADMIN,

    /**
     * Event is visible to the object owner, the tenant admin, and the global admin.
     */
    OWNER,

    /**
     * Event is visible to the user, the owner of the associated object, the tenant admin, and the global admin.
     */
    USER;
}

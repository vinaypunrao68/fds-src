/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

import java.util.Objects;

/**
 * Represent a user in the system
 */
public class User extends AbstractResource<Long> {

    private final Long roleId;
    private final Tenant tenant;

    private User() {
    	this( 0L, "None", 0L, null );
    }
    
    /**
     *
     * @param uid the user ID
     * @param name the user login name
     * @param roleDescriptor the role
     * @param tenant the tenant the user belongs to
     */
    public User( Long uid, String name, Long roleId, Tenant tenant ) {
        super( uid, name );
        this.roleId = roleId;
        this.tenant = tenant;
    }

    /**
     * @return the users's role descriptors.
     */
    public Long getRoleId() {
        return roleId;
    }

    /**
     *
     * @return the tenant the user belongs to.
     */
    public Tenant getTenant() {
        return tenant;
    }

    @Override
    public boolean equals( Object o ) {
        if ( this == o ) { return true; }
        if ( !(o instanceof User) ) { return false; }
        if ( !super.equals( o ) ) { return false; }
        final User user = (User) o;
        return Objects.equals( roleId, user.roleId ) &&
               Objects.equals( tenant, user.tenant );
    }

    @Override
    public int hashCode() {
        return Objects.hash( super.hashCode(), roleId, tenant );
    }

    @Override
    public String toString() {
        final StringBuilder sb = new StringBuilder( "User{" );
        sb.append( "roleId=" ).append( roleId );
        sb.append( ", tenant=" ).append( tenant );
        sb.append( '}' );
        return sb.toString();
    }
}

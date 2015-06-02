/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.model;

import java.util.Objects;

public class User extends AbstractResource {

    private final RoleDescriptor roleDescriptor;
    private final Tenant tenant;

    /**
     *
     * @param uid the user ID
     * @param name
     * @param roleDescriptor
     * @param tenant
     */
    public User( ID uid, String name, RoleDescriptor roleDescriptor, Tenant tenant ) {
        super( uid, name );
        this.roleDescriptor = roleDescriptor;
        this.tenant = tenant;
    }

    public RoleDescriptor getRoleDescriptor() {
        return roleDescriptor;
    }

    public Tenant getTenant() {
        return tenant;
    }

    @Override
    public boolean equals( Object o ) {
        if ( this == o ) { return true; }
        if ( !(o instanceof User) ) { return false; }
        if ( !super.equals( o ) ) { return false; }
        final User user = (User) o;
        return Objects.equals( roleDescriptor, user.roleDescriptor ) &&
               Objects.equals( tenant, user.tenant );
    }

    @Override
    public int hashCode() {
        return Objects.hash( super.hashCode(), roleDescriptor, tenant );
    }

    @Override
    public String toString() {
        final StringBuilder sb = new StringBuilder( "User{" );
        sb.append( "roleDescriptor=" ).append( roleDescriptor );
        sb.append( ", tenant=" ).append( tenant );
        sb.append( '}' );
        return sb.toString();
    }
}

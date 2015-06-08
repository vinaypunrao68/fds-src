/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

import java.util.ArrayList;
import java.util.EnumSet;
import java.util.List;

/**
 * Standard Role descriptors and their supported feature lists
 */
public enum Role implements RoleDescriptor {

    ADMIN(Feature.SYS_MGMT, Feature.TENANT_MGMT, Feature.USER_MGMT, Feature.VOL_MGMT),
    TENANT_ADMIN(Feature.TENANT_MGMT, Feature.USER_MGMT, Feature.VOL_MGMT),
    USER(Feature.VOL_MGMT, Feature.USER_MGMT);

    private final EnumSet<Feature> features;

    private Role(){
    	this( Feature.VOL_MGMT );
    }

    Role( Feature f1, Feature... features ) {
        this.features = EnumSet.of( f1, features );
    }

    @Override
    public List<Feature> getFeatures() {
        return new ArrayList<>( features );
    }
}

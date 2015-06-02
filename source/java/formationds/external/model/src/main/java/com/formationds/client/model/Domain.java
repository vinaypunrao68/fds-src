/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.model;

import java.util.ArrayList;
import java.util.List;

public class Domain extends AbstractResource {

    private final List<Node> nodes = new ArrayList<>();

    public Domain( ID id, String key ) {
        super( id, key );
    }

    public List<Node> getNodes() {
        return nodes;
    }
    //    /**
    //     *
    //     * @return the map of tenants that are part of this domain.
    //     */
    //    @Lazy
    //    Map<TenantId, Tenant> getTenants();
}

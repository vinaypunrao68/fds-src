/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

import java.util.ArrayList;
import java.util.List;

public class Domain extends AbstractResource<Long> {

    private List<Node> nodes = new ArrayList<>();
    private final String site;

    private Domain() {
        this( 0L, "fds", "local" );
    }

    public Domain( String key, String site ) {
        this( null, key, site );
    }

    public Domain( Long id, String key, String site ) {
        super( id, key );
        this.site = site;
    }

    public String getSite() {
        return site;
    }

    /**
     * @return the list of nodes for this domain
     */
    public List<Node> getNodes() {

        if ( nodes == null ) {
            nodes = new ArrayList<>();
        }

        return nodes;
    }

    //    /**
    //     *
    //     * @return the map of tenants that are part of this domain.
    //     */
    //    @Lazy
    //    Map<TenantId, Tenant> getTenants();
}

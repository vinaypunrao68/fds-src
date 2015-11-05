/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

import java.util.ArrayList;
import java.util.List;

public class Domain extends AbstractResource<Integer> {

	public enum DomainState { UP, DOWN, UNKNOWN };
	
    private List<Node> nodes = new ArrayList<>();
    private final String site;
    private DomainState state;

    private Domain() {
        this( 0, "fds", "local", DomainState.UNKNOWN );
    }

    public Domain( String key, String site ) {
        this( null, key, site, DomainState.UNKNOWN );
    }

    public Domain( Integer id, String key, String site, DomainState state ) {
        super( id, key );
        this.site = site;
        this.state = state;
    }

    public String getSite() {
        return site;
    }
    
    public DomainState getState() {
    	return this.state;
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

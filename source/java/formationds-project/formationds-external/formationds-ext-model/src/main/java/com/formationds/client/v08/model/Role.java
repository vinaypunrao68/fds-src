/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

import java.util.ArrayList;
import java.util.List;

/**
 * Standard Role descriptors and their supported feature lists
 */
public class Role extends AbstractResource<Long>{

    private final List<Feature> features;

    private Role(){
    	this( 0L, "None", Feature.VOL_MGMT );
    }

    Role( Long id, String name, Feature f1, Feature... features ) {
    	super(id, name);
    	
        this.features = new ArrayList<Feature>();
        
        this.features.add( f1 );
        
        for ( Feature feature : features ){
        	this.features.add( feature );
        }
    }

    public List<Feature> getFeatures() {
        return this.features;
    }
}

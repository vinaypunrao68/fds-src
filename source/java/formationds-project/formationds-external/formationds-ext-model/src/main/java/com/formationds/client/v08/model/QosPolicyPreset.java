/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

import java.util.Objects;

public class QosPolicyPreset extends QosPolicyBase {
    private Long id;
    private String name;

    /**
     *
     * @param id the id of the qos policy preset
     * @param priority the priority
     * @param iopsMin the minimum iops setting
     * @param iopsMax the maximum iops setting
     */
    public QosPolicyPreset( Long id, String name, int priority, int iopsMin, int iopsMax ) {
        super(priority, iopsMin, iopsMax );
        this.id = id;
        this.name = name;
    }

    /**
     *
     * @param priority the priority
     * @param iopsMin the minimum iops setting
     * @param iopsMax the maximum iops setting
     */
    public QosPolicyPreset( int priority, int iopsMin, int iopsMax ) {
        super(priority, iopsMin, iopsMax );
    }

    public Long getId() {
        return id;
    }

    public void setId( Long id ) {
        this.id = id;
    }

    public String getName(){
    	return this.name;
    }
    
    public void setName( String name ){
    	this.name = name;
    }
    
    public boolean isEquals( Object o ) {
        if ( this == o ) { return true; }
        if ( !(o instanceof QosPolicyBase) ) { return false; }
        final QosPolicyPreset that = (QosPolicyPreset) o;
        return Objects.equals( id, that.id );
    }

    @Override
    public boolean equals( Object o ) {
        if ( this == o ) { return true; }
        if ( !(o instanceof QosPolicyPreset) ) { return false; }
        if ( !super.equals( o ) ) { return false; }
        final QosPolicyPreset that = (QosPolicyPreset) o;
        return Objects.equals( id, that.id );
    }

    @Override
    public int hashCode() {
        return Objects.hash( super.hashCode(), id );
    }
}

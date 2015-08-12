/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

import java.util.Objects;

public class Service extends AbstractResource<Long> {

    public enum ServiceState {
        RUNNING,
        NOT_RUNNING,
        LIMITED,
        DEGRADED,
        UNEXPECTED_EXIT,
        ERROR,
        UNREACHABLE,
        INITIALIZING,
        SHUTTING_DOWN,
        STANDBY;         // To reflect PM state when the node is down
    }
    
    public static class ServiceStatus{
    	
    	private ServiceState state;
    	private final String description;
    	private final int errorCode;
    	
    	public ServiceStatus( ServiceState state ){
    		this( state, "", 0);
    	}
    	
    	public ServiceStatus( ServiceState state, String description, int errorCode ){
    		this.state = state;
    		this.description = description;
    		this.errorCode = errorCode;
    	}
    	
    	public ServiceState getServiceState(){
    		return state;
    	}
    	
    	public void setServiceState( ServiceState state ){
    		this.state = state;
    	}
    	
    	public int getErrorCode(){
    		return errorCode;
    	}
    	
    	public String getDescription(){
    		return description;
    	}
    	
    }

    private ServiceType type;
    private int port;

    private ServiceStatus status;

    protected Service() {}

    public Service( ServiceType type, int port ) {
        super( null, type.name(), Integer.toString( port ) );
    }

    public Service( ServiceType type, int port, ServiceStatus status ) {
        super( null, type.name() );
        this.type = type;
        this.port = port;
        this.status = status;
    }

    public Service( Long id, ServiceType type, int port, ServiceStatus status ) {
        super( id, type.name() );
        this.type = type;
        this.port = port;
        this.status = status;
    }

    public ServiceType getType() {
        return type;
    }

    public int getPort() {
        return port;
    }

    public ServiceStatus getStatus() {
        return status;
    }

    public void setStatus( ServiceStatus status ) {
        this.status = status;
    }

    public void setPort( int port ) {
        this.port = port;
    }

    public void setType( ServiceType type ) {
        this.type = type;
    }

    @Override
    public boolean equals( Object o ) {
        if ( this == o ) { return true; }
        if ( !(o instanceof Service) ) { return false; }
        if ( !super.equals( o ) ) { return false; }
        final Service service = (Service) o;
        return Objects.equals( port, service.port ) &&
               Objects.equals( type, service.type ) &&
               Objects.equals( status, service.status );
    }

    @Override
    public int hashCode() {
        return Objects.hash( super.hashCode(), type, port, status );
    }
}

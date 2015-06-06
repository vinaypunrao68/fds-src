package com.formationds.client.v08.model;

import java.util.ArrayList;
import java.util.List;

public class SystemStatus {

	private List<SystemHealth> status;
	private HealthState overall;
	
	public List<SystemHealth> getStatus() {
		return status;
	}
	
	public void setStatus(List<SystemHealth> status) {
		this.status = status;
	}
	
	public void addStatus( SystemHealth aStatus ) {
		
		if ( this.status == null ) {
			this.status = new ArrayList<SystemHealth>();
		}
		
		if ( !this.status.contains( aStatus ) ) {
			this.status.add( aStatus );
		}
	}
	
	public HealthState getOverall() {
		return overall;
	}
	
	public void setOverall(HealthState overall) {
		this.overall = overall;
	}
}

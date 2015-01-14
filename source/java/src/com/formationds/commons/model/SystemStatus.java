package com.formationds.commons.model;

import java.util.ArrayList;
import java.util.List;

import com.formationds.commons.model.abs.ModelBase;
import com.formationds.commons.model.type.HealthState;

public class SystemStatus extends ModelBase {

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

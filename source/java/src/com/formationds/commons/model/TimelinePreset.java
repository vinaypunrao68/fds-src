package com.formationds.commons.model;

import java.util.ArrayList;
import java.util.List;

import com.formationds.commons.model.abs.ModelBase;

public class TimelinePreset extends ModelBase{

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;

	private Long commitLogRetention;
	private Long uuid;
	private String name;
	
	private List<SnapshotPolicy> policies;
	
	public Long getUuid() {
		return uuid;
	}
	
	public void setUuid( Long anId ) {
		uuid = anId;
	}
	
	public String getName() {
		return name;
	}
	
	public void setName( String aName ) {
		name = aName;
	}
	
	public Long getCommitLogRetention() {
		return commitLogRetention;
	}
	
	public void setCommitLogRetention( Long time ) {
		commitLogRetention = time;
	}
	
	public List<SnapshotPolicy> getPolicies() {
		
		if ( policies == null ) {
			policies = new ArrayList<SnapshotPolicy>();
		}
		return policies;
	}
	
	public void setPolicies( List<SnapshotPolicy> somePolicies ) {
		policies = somePolicies;
	}
}

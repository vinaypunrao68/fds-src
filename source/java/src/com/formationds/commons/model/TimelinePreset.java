package com.formationds.commons.model;

import java.util.List;

import com.formationds.commons.model.abs.ModelBase;

public class TimelinePreset extends ModelBase{

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;

	private Long commitLogRetention;
	
	private List<SnapshotPolicy> policies;
	
	public Long getCommitLogRetention() {
		return commitLogRetention;
	}
	
	public void setCommitLogRetention( Long time ) {
		commitLogRetention = time;
	}
	
	public List<SnapshotPolicy> getPolicies() {
		return policies;
	}
	
	public void setPolicies( List<SnapshotPolicy> somePolicies ) {
		policies = somePolicies;
	}
}

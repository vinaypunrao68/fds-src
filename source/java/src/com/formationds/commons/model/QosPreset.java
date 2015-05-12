package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;

public class QosPreset extends ModelBase{

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	private Integer priority;
	private Long sla;
	private Long limit;
	private Long uuid = -1L;
	private String name;

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
	
	public Integer getPriority() {
		return priority;
	}

	public void setPriority(Integer priority) {
		this.priority = priority;
	}

	public Long getSla() {
		return sla;
	}

	public void setSla(Long sla) {
		this.sla = sla;
	}

	public Long getLimit() {
		return limit;
	}

	public void setLimit(Long limit) {
		this.limit = limit;
	}
}

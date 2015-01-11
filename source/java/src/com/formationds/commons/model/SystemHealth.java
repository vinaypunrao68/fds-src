package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;
import com.formationds.commons.model.type.HealthState;

public class SystemHealth extends ModelBase{

	public enum CATEGORY{ SERVICES, CAPACITY, FIREBREAK };
	
	/**
	 * 
	 */
	private static final long serialVersionUID = 9189384454692190607L;

	private HealthState state;
	private CATEGORY category;
	private String message;
	
	public SystemHealth(){
		super();
	}
	
	public HealthState getState() {
		return state;
	}

	public void setState(HealthState state) {
		this.state = state;
	}

	public CATEGORY getCategory() {
		return category;
	}

	public void setCategory(CATEGORY category) {
		this.category = category;
	}

	public String getMessage() {
		return message;
	}

	public void setMessage(String message) {
		this.message = message;
	}
}

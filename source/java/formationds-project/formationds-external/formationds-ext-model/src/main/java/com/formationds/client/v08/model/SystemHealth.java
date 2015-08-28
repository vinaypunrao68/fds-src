package com.formationds.client.v08.model;

public class SystemHealth {

	/**
	 * 
	 */
	private static final long serialVersionUID = 9189384454692190607L;

	private HealthState state;
	private String category;
	private String message;
	
	public SystemHealth(){
		super();
	}

	public SystemHealth( String category, HealthState state, String message ) {
		this.state = state;
		this.category = category;
		this.message = message;
	}

	public HealthState getState() {
		return state;
	}

	public void setState(HealthState state) {
		this.state = state;
	}

	public String getCategory() {
		return category;
	}

	public void setCategory( String category ) {
		this.category = category;
	}

	public String getMessage() {
		return message;
	}

	public void setMessage(String message) {
		this.message = message;
	}
}

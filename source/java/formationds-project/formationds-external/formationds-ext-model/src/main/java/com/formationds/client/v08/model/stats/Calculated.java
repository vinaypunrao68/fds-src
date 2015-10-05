/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model.stats;

/**
 * @author ptinius
 */
public class Calculated {
	
	private static final long serialVersionUID = -3524304479018347948L;

	private String key;
	private Double value;
	
	public Calculated( String key, Double value ){
		this.key = key;
		this.value = value;
	}
  
	public String getKey(){
		return key;
	}
	
	public void setKey( String key ){
		this.key = key;
	}
	
	public Double getValue(){
		return value;
	}
	
	public void setValue( Double value ){
		this.value = value;
	}
	
}

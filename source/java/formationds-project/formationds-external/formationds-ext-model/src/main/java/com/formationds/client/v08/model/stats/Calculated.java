/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model.stats;

/**
 * @author ptinius
 */
public class Calculated {
	
	private static final long serialVersionUID = -3524304479018347948L;

	public static final String IOPS_CONSUMED = "dailyAverage";
	public static final String TOTAL_CAPACITY = "totalCapacity";
	public static final String CONSUMED_BYTES = "total";
	public static final String AVERAGE_IOPS = "average";
	public static final String PERCENTAGE = "percentage";
	public static final String RATIO = "ratio";
	public static final String TO_FULL = "toFull";
	
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

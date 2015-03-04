package com.formationds.commons.model.calculated.performance;

import com.formationds.commons.model.abs.Calculated;

public class PercentageConsumed extends Calculated{

	/**
	 * 
	 */
	private static final long serialVersionUID = -7890151887533704385L;
	private Double percentage;
	
	public PercentageConsumed(){}
	
	public Double getPercentage(){
		return percentage;
	}
	
	public void setPercentage( Double aPercentage ){
		this.percentage = aPercentage;
	}
}

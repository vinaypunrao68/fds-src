package com.formationds.commons.model.calculated.capacity;

import com.formationds.commons.model.abs.Calculated;
import com.google.gson.annotations.SerializedName;

public class AverageIOPs extends Calculated {

	private static final long serialVersionUID = 7937123375009213316L;
	
	@SerializedName( "averageIops" )
	private Double average;
	
	/**
	 * default constructor
	 */
	public AverageIOPs() {
	}

	/**
	 * @return Returns the {@link Double} representing the total capacity
	 */
	public Double getAverage() {
		return average;
	}

	/**
	 * @param total the {@link Double} representing the total capacity.
	 */
	public void setAverage( final Double average ) {
		this.average = average;
	}
}

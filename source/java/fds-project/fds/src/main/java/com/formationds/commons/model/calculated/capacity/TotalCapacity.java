package com.formationds.commons.model.calculated.capacity;

import com.formationds.commons.model.abs.Calculated;
import com.google.gson.annotations.SerializedName;

public class TotalCapacity
extends Calculated {
	private static final long serialVersionUID = 4144617532432278800L;

	@SerializedName( "total_capacity" )
	private Double totalCapacity;

	/**
	 * default constructor
	 */
	public TotalCapacity() {
	}

	/**
	 * @return Returns the {@link java.lang.Integer} representing the duration
	 * until capacity is 100% (full)
	 */
	public Double getTotalCapacity() {
		return totalCapacity;
	}

	/**
	 * @param capacity the {@link java.lang.Integer}  representing the duration
	 *               until capacity is 100% (full)
	 */
	public void setTotalCapacity( final Double capacity ) {
		this.totalCapacity = capacity;
	}
}


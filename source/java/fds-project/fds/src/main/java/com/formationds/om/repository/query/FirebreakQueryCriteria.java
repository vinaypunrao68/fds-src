package com.formationds.om.repository.query;

public class FirebreakQueryCriteria extends MetricQueryCriteria {

	/**
	 *
	 */
	private static final long serialVersionUID = 250753918037627684L;

	// this field indicates the number of recent results requested per context (volume)
	private Integer mostRecentResults;

	// this field will specify that you want the volume size as the y value and not the firebreak type ordinal
	private Boolean useSizeForValue;

	public FirebreakQueryCriteria(QueryType type) {
		super(type);
	}

	public Integer getMostRecentResults(){
		return mostRecentResults;
	}

	public void setMostRecentResults( Integer numResults ){
		this.mostRecentResults = numResults;
	}

	public Boolean isUseSizeForValue(){
		return useSizeForValue;
	}

	public void setUseSizeForValue( Boolean shouldI ){
		this.useSizeForValue = shouldI;
	}
}

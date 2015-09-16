package com.formationds.client.v08.model.stats.query;

public class OrderBy {

	private String fieldName;
	private boolean ascending;
	
	public OrderBy(){
		
	}
	
	public OrderBy( String aFieldName, boolean isAscending ){
		
		this.fieldName = aFieldName;
		this.ascending = isAscending;
	}
	
	public String getFieldName(){
		return fieldName;
	}
	
	public void setFieldName( String aFieldName ){
		this.fieldName = aFieldName;
	}
	
	public boolean isAscending(){
		return ascending;
	}
	
	public void setAscending( boolean isAscending ){
		this.ascending = isAscending;
	}
	
	@Override
	public boolean equals(Object obj) {

		if ( !(obj instanceof OrderBy ) ){
			return false;
		}
		
		OrderBy checkOrderBy = (OrderBy)obj;
		
		return ( checkOrderBy.isAscending() == isAscending() && checkOrderBy.getFieldName().equals( getFieldName() ) );
	}
}

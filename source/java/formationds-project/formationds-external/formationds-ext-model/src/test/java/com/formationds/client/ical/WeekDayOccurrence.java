package com.formationds.client.ical;

import java.io.Serializable;

public class WeekDayOccurrence implements Serializable {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1329092205313945221L;

	private iCalWeekDays weekday;
	private Integer occurrence;
	
	public void setWeekday( iCalWeekDays aDay ){
		weekday = aDay;
	}
	
	public iCalWeekDays getWeekday(){
		return weekday;
	}
	
	public void setOccurrence( Integer anOccurrence ){
		occurrence = anOccurrence;
	}
	
	public Integer getOccurrence(){
		return occurrence;
	}
	
	@Override
	public String toString() {
		StringBuilder sb = new StringBuilder();
		
		if ( getOccurrence() != null && !getOccurrence().equals( 0 ) ){
			sb.append( getOccurrence() );
		}
		
		sb.append( getWeekday().toString() );
		
		return sb.toString();
	}
}

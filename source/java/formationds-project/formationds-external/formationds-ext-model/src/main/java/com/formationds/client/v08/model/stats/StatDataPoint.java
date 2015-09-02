package com.formationds.client.v08.model.stats;

import org.json.JSONObject;

public class StatDataPoint {

	public static final String REPORT_TIME = "reportTime";
	public static final String METRIC_NAME = "metricName";
	public static final String METRIC_VALUE = "metricValue";
	public static final String COLLECTION_PERIOD = "collectionPeriod";
	public static final String COLLECTION_TIME_UNIT = "collectionTimeUnit";
	public static final String CONTEXT_ID = "contextId";
	public static final String CONTEXT_TYPE_STR = "contextType";
	
	public enum TIME_UNITS{
		MILLISECONDS,
		SECONDS,
		MINUTES,
		HOURS,
		WEEKS,
		DAYS,
		YEARS
	};
	
	/**
	 * This is the time the statistic was reported
	 */
	private Long reportTime = 0L;
	
	/**
	 * This is the name of the metric.  This will act as a key for most searches and is
	 * based mostly upon the enumeration of stat types found in the definition:
	 * com.formationds.commons.model.type.Metrics
	 */
	private String metricName = "UNKNOWN";
	
	/**
	 * The actual metric value.  This should be a raw value that is not
	 * an average or other computation
	 */
	private Double metricValue = 0.0;
	
	/**
	 * The period of time it took to collect the value.  In other words,
	 * if the value is a cumulative value, this would be the amount of time
	 * over which the summation was done.
	 */
	private Long collectionPeriod = 0L;
	
	/**
	 * This is an integer instead of an enum to promote the cross language 
	 * concerns.  Descriptions of what each integer means in terms of time period can be found in
	 * this file as static final members.
	 */
	private TIME_UNITS collectionTimePeriod = TIME_UNITS.MILLISECONDS;
	
	/**
	 * The internal FDS ID for the item that the stat is about.  
	 */
	private Long contextId = -1L;
	
	/**
	 * The type (or system noun) that describes what the stat is about
	 * 0 = Volume
	 * 1 = Node
	 * 2 = Service
	 */
	private ContextType contextType;
	
	public StatDataPoint(){}
	
	public StatDataPoint( Long reportTime, 
						  String metricName,
						  Double metricValue,
						  Long collectionPeriod,
						  TIME_UNITS collectionTimePeriod,
						  Long contextId,
						  ContextType contextType ){
		
		this.reportTime = reportTime;
		this.metricName = metricName;
		this.metricValue = metricValue;
		this.collectionPeriod = collectionPeriod;
		this.contextId = contextId;
		this.contextType = contextType;
		this.collectionTimePeriod = collectionTimePeriod;
	}

	public Long getReportTime() {
		return reportTime;
	}

	public void setReportTime( Long reportTime ) {
		this.reportTime = reportTime;
	}

	public String getMetricName() {
		return metricName;
	}

	public void setMetricName( String metricName ) {
		this.metricName = metricName;
	}

	public Double getMetricValue() {
		return metricValue;
	}

	public void setMetricValue( Double metricValue ) {
		this.metricValue = metricValue;
	}

	public Long getCollectionPeriod() {
		return collectionPeriod;
	}

	public void setCollectionPeriod( Long collectionPeriod ) {
		this.collectionPeriod = collectionPeriod;
	}
	
	public TIME_UNITS getCollectionTimeUnit(){
		return collectionTimePeriod;
	}
	
	public void setCollectionTimeUnit( TIME_UNITS timeUnit ){
		this.collectionTimePeriod = timeUnit;
	}

	public Long getContextId() {
		return contextId;
	}

	public void setContextId( Long contextId ) {
		this.contextId = contextId;
	}

	public ContextType getContextType() {
		return contextType;
	}

	public void setContextType( ContextType contextType ) {
		this.contextType = contextType;
	}
	
	/**
	 * Built in marshaling to json
	 * @return
	 */
	public String toJson(){
		
		JSONObject json = new JSONObject();
		
		json.put( REPORT_TIME, getReportTime() );
		json.put( METRIC_NAME, getMetricName() );
		json.put( METRIC_VALUE, getMetricValue() );
		json.put( COLLECTION_PERIOD, getCollectionPeriod() );
		json.put( CONTEXT_ID, getContextId() );
		json.put( CONTEXT_TYPE_STR, getContextType().name() );
		json.put( COLLECTION_TIME_UNIT, getCollectionTimeUnit().name() );
		
		String rtn = json.toString();
		return rtn;
	}
	
	/**
	 * built in marshaling from json.
	 * @param jsonStr
	 * @return
	 */
	public static StatDataPoint fromJson( String jsonStr ){
		
		JSONObject json = new JSONObject( jsonStr );
		
		StatDataPoint datapoint = new StatDataPoint();
		
		datapoint.setReportTime( json.getLong( REPORT_TIME ) );
		datapoint.setCollectionPeriod( json.getLong( COLLECTION_PERIOD ) );
		datapoint.setContextId( json.getLong( CONTEXT_ID ) );
		datapoint.setContextType( ContextType.valueOf( json.getString( CONTEXT_TYPE_STR ) ) );
		datapoint.setMetricName( json.getString( METRIC_NAME ) );
		datapoint.setMetricValue( json.getDouble( METRIC_VALUE ) );
		datapoint.setCollectionTimeUnit( TIME_UNITS.valueOf( json.getString( COLLECTION_TIME_UNIT ) ) );
		
		return datapoint;
	}
}

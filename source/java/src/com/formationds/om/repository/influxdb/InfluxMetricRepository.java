/**
 * Copyright (c) 2014 Formation Data Systems.  All rights reserved.
 */

package com.formationds.om.repository.influxdb;

import com.formationds.apis.VolumeStatus;
import com.formationds.commons.model.Volume;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.type.Metrics;
import com.formationds.om.repository.MetricRepository;
import com.formationds.om.repository.query.QueryCriteria;

import org.influxdb.dto.Serie;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Properties;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

/**
 *
 */
public class InfluxMetricRepository extends InfluxRepository<VolumeDatapoint, Long> implements MetricRepository {

    public static final String VOL_SERIES_NAME = "volume_metrics";
    public static final String VOL_ID_COLUMN_NAME = "volume_id";

    /**
     * the static list of metric names store in the influxdb database.
     */
    public static final List<String> VOL_METRIC_NAMES = Collections.unmodifiableList( getMetricNames() );

    public static final InfluxDatabase DEFAULT_METRIC_DB =
        new InfluxDatabase.Builder( "om-metricdb" )
                          .addShardSpace( "default", "30d", "1d", "/.*/", 1, 1 )
                          .build();

    /**
     * @return the list of metric names in the order they are stored.
     */
    public static List<String> getMetricNames() {
        List<String> metricNames = Arrays.stream( Metrics.values() )
                                         .map( Enum::name )
                                         .collect( Collectors.toList() );

        List<String> volMetricNames = new ArrayList<>();
        volMetricNames.add( VOL_ID_COLUMN_NAME );
        volMetricNames.addAll( metricNames );

        return volMetricNames;
    }

    /**
     *
     * @param url
     * @param adminUser
     * @param adminCredentials
     */
    public InfluxMetricRepository( String url, String adminUser, char[] adminCredentials ) {
        super( url, adminUser, adminCredentials );
    }

    /**
     * @param properties the connection properties
     */
    @Override
    synchronized protected void open( Properties properties ) {
        // command is silently ignored if the database already exists.
        if ( !super.createDatabase( DEFAULT_METRIC_DB ) ) {
            logger.debug( "Database " + DEFAULT_METRIC_DB.getName() + " already exists." );
        }
        super.open( properties );
    }

    @Override
    public String getEntityName() {
        return VOL_SERIES_NAME;
    }

    @Override
    public String getTimestampColumnName() {
        return super.getTimestampColumnName();
    }

    @Override
    public Optional<String> getVolumeNameColumnName() {
        return Optional.empty();
    }

    @Override
    public Optional<String> getVolumeIdColumnName() {
        return Optional.of( VOL_ID_COLUMN_NAME );
    }

    /**
     * @throws UnsupportedOperationException persisting individual metrics is not supported for the
     * Influx Metric Repository
     */
    @Override
    protected VolumeDatapoint doPersist( VolumeDatapoint entity ) {
        throw new UnsupportedOperationException( "Persisting individual metrics is not supported for the Influx Metric repository" );
    }

    @Override
    protected List<VolumeDatapoint> doPersist( Collection<VolumeDatapoint> entities ) {
        Object[] metricValues = new Object[ VOL_METRIC_NAMES.size() ];

        // TODO: currently the collection of VolumeDatapoint objects is a list of individual data points
        // and may contain any number of volumes and timestamps.  Ironically, the AM receives the datapoints
        // exactly as we need  them here, but it then splits them in JsonStatisticsFormatter
        List<VolumeDatapoint> vdps = (entities instanceof List ? (List)entities : new ArrayList<>( entities ));

        // timestamp, map<volname, List<vdp>>>
        Map<Long, Map<String, List<VolumeDatapoint>>> orderedVDPs;
        orderedVDPs = vdps.stream()
                           .collect( Collectors.groupingBy( VolumeDatapoint::getTimestamp,
                                                            Collectors.groupingBy( VolumeDatapoint::getVolumeName ) ) );

        for (Map.Entry<Long,Map<String,List<VolumeDatapoint>>> e : orderedVDPs.entrySet())
        {
            Long ts = e.getKey();
            Map<String, List<VolumeDatapoint>> volumeDatapoints = e.getValue();

            for (Map.Entry<String, List<VolumeDatapoint>> e2 : volumeDatapoints.entrySet()) {
                String volid = e2.getKey();

                metricValues[0] = volid;

                for (VolumeDatapoint vdp : e2.getValue())
                {
                    // TODO: figure out what metric it maps to, then figure out its position in
                    // the values array.
                }

                Serie serie = new Serie.Builder( VOL_SERIES_NAME )
                                  .columns( VOL_METRIC_NAMES.toArray( new String[VOL_METRIC_NAMES.size()] ) )
                                  .values()
                                  .build();
            }
        }
        return vdps;
    }

    @Override
    protected void doDelete( VolumeDatapoint entity ) {
        // TODO: not supported yet
        throw new UnsupportedOperationException( "Delete not yet supported" );
    }

    @Override
    public VolumeDatapoint findById( Long aLong ) {
        // TODO: not sure how meaningful this is in the context of influxdb?
        return null;
    }

    @Override
    public List<VolumeDatapoint> findAll() {
        QueryCriteria criteria = new QueryCriteria();
        List<VolumeDatapoint> results = query( criteria );
        
        return results;
    }

    @Override
    public long countAll() {
        return 0;
    }

    @Override
    public long countAllBy( VolumeDatapoint entity ) {
        return 0;
    }
    
    /**
     * Method to create a string from the query object that matches influx format
     * 
     * @param queryCriteria
     * @return
     */
    protected String formulateQueryString( QueryCriteria queryCriteria ){
    	
    	StringBuilder sb = new StringBuilder();
        
    	String prefix = SELECT + " * " + FROM + " " + getEntityName();
    	sb.append( prefix );
    
    	if ( queryCriteria.getRange() != null && 
    			queryCriteria.getContexts() != null && queryCriteria.getContexts().size() > 0 ) {
    		
    		sb.append( " " + WHERE );
    	}
    	
    	// do time range
    	if ( queryCriteria.getRange() != null ){
    	
	    	String time = " ( " + getTimestampColumnName() + " >= " + queryCriteria.getRange().getStart() + " " + AND + 
	    			" " + getTimestampColumnName() + " <= " + queryCriteria.getRange().getEnd() + " ) ";
	    	
	    	sb.append( time );
    	}
    	
    	if ( queryCriteria.getContexts() != null && queryCriteria.getContexts().size() > 0 ) {
    	
    		sb.append( AND + " ( " );
    		
    		Iterator<Volume> contextIt = queryCriteria.getContexts().iterator();
    		
    		while( contextIt.hasNext() ) {
    			
    			Volume volume = contextIt.next();
    			
    			sb.append( getVolumeIdColumnName().get() + " = " + volume.getId() );
    			
    			if ( contextIt.hasNext() ){
    				sb.append( " " + OR + " " );
    			}
    		}
    		
    		sb.append( " )" );
    	}
    	
    	return sb.toString();
    }
    
    /**
     * Convert an influxDB return type into VolumeDatapoints that we can use
     * 
     * @param series
     * @return
     */
    protected List<VolumeDatapoint> convertSeriesToPoints( List<Serie> series ) {
    	
    	final List<VolumeDatapoint> datapoints = new ArrayList<VolumeDatapoint>();
    	
    	// we expect rows from one and only one series.  If there are more, we'll only use
    	// the first one
    	if ( series == null || series.size() == 0 ) {
    		return datapoints;
    	}
    	
    	List<Map< String, Object>> rowList = series.get( 0 ).getRows();
    	
    	for ( Map<String, Object> row : rowList ) {
    		
    		// get the timestamp
    		Object timestampO = row.get( getTimestampColumnName() );
    		Object volumeIdO = row.get( getVolumeIdColumnName().get() );
    		Object volumeNameO = row.get( getVolumeNameColumnName().get() );
    		
    		// we expect a value for all of these fields.  If not, bail
    		if ( timestampO == null || volumeIdO == null || volumeNameO == null ) {
    			continue;
    		}
    		
    		Long timestamp = Long.parseLong(timestampO.toString() );
    		String volumeName = volumeIdO.toString();
    		String volumeId = volumeIdO.toString();
    		
    		row.forEach( ( key, value ) -> {
    		
    			// If we run across a column for metadata we just skip it.
    			// we're only interested in the stats columns at this point
    			if ( key.equals( getTimestampColumnName() ) ||
    				key.equals( getVolumeIdColumnName().get() ) || 
    				key.equals( getVolumeNameColumnName().get() ) ||
    				value == null ) {
    				return;
    			}
    			
    			Double numberValue = Double.parseDouble( value.toString() );
    			
    			VolumeDatapoint point = new VolumeDatapoint( timestamp, volumeId, volumeName, key, numberValue );
    			datapoints.add( point );
    			
    		});
    	} // for each row
    	
    	return datapoints;
    }

    @Override
    public List<? extends VolumeDatapoint> query( QueryCriteria queryCriteria ) {
    	
    	// get the query string
    	String queryString = formulateQueryString( queryCriteria );
    	
    	// execute the query
    	List<Serie> series = getConnection().getDBReader().query( queryString, TimeUnit.MILLISECONDS );
    	
    	// convert from influxdb format to FDS model format
    	List<VolumeDatapoint> datapoints = convertSeriesToPoints( series );
    	
        return datapoints;
    }

    @Override
    public Double sumLogicalBytes() {
        return null;
    }

    @Override
    public Double sumPhysicalBytes() {
        return null;
    }

    @Override
    public VolumeDatapoint mostRecentOccurrenceBasedOnTimestamp( String volumeName, Metrics metric ) {
        return null;
    }

    @Override
    public VolumeDatapoint mostRecentOccurrenceBasedOnTimestamp( Long volumeId, Metrics metric ) {
        return null;
    }

    @Override
    public VolumeDatapoint leastRecentOccurrenceBasedOnTimestamp( Long volumeId, Metrics metric ) {
        return null;
    }

    @Override
    public VolumeDatapoint leastRecentOccurrenceBasedOnTimestamp( String volumeName, Metrics metric ) {
        return null;
    }

    @Override
    public Optional<VolumeStatus> getLatestVolumeStatus( Long volumeId ) {
        return null;
    }

    @Override
    public Optional<VolumeStatus> getLatestVolumeStatus( String volumeName ) {
        return null;
    }
}

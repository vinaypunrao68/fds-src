package com.formationds.om.webkit.rest;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

import javax.persistence.EntityManager;

import org.apache.thrift.TException;
import org.eclipse.jetty.server.Request;
import org.h2.tools.ConvertTraceFile;
import org.json.JSONArray;
import org.json.JSONObject;

import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import FDS_ProtocolInterface.FDSP_MgrIdType;
import FDS_ProtocolInterface.FDSP_MsgHdrType;
import FDS_ProtocolInterface.FDSP_Node_Info_Type;
import FDS_ProtocolInterface.FDSP_NodeState;

import com.formationds.apis.VolumeDescriptor;
import com.formationds.commons.model.DateRange;
import com.formationds.commons.model.Series;
import com.formationds.commons.model.SystemHealth;
import com.formationds.commons.model.Volume;
import com.formationds.commons.model.abs.Context;
import com.formationds.commons.model.builder.VolumeBuilder;
import com.formationds.commons.model.calculated.capacity.CapacityConsumed;
import com.formationds.commons.model.calculated.capacity.CapacityFull;
import com.formationds.commons.model.calculated.capacity.CapacityToFull;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.type.HealthState;
import com.formationds.commons.model.type.Metrics;
import com.formationds.commons.model.type.StatOperation;
import com.formationds.om.repository.SingletonRepositoryManager;
import com.formationds.om.repository.helper.FirebreakHelper;
import com.formationds.om.repository.helper.QueryHelper;
import com.formationds.om.repository.helper.SeriesHelper;
import com.formationds.om.repository.query.MetricQueryCriteria;
import com.formationds.om.repository.query.builder.MetricCriteriaQueryBuilder;
import com.formationds.om.repository.query.builder.MetricQueryCriteriaBuilder;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.ConfigurationApi;
import com.formationds.util.SizeUnit;

public class SystemHealthStatus implements RequestHandler {

	private final FDSP_ConfigPathReq.Iface legacyConfig;
	private final ConfigurationApi configApi;
	private final Authorizer authorizer;
	private final AuthenticationToken token;
	
	private final String SERVICES_GOOD = "l_services_good";
    private final String SERVICES_OKAY = "l_services_not_good";
    private final String SERVICES_BAD = "l_services_bad";
    
    private final String CAPACITY_GOOD = "l_capacity_good";
    private final String CAPACITY_OKAY_THRESHOLD = "l_capacity_not_good_threshold";
    private final String CAPACITY_OKAY_RATE = "l_capacity_not_good_rate";
    private final String CAPACITY_BAD_THRESHOLD = "l_capacity_bad_threshold";
    private final String CAPACITY_BAD_RATE = "l_capacity_bad_rate";
    
    private final String FIREBREAK_GOOD = "l_firebreak_good";
    private final String FIREBREAK_OKAY = "l_firebreak_not_good";
    private final String FIREBREAK_BAD = "l_firebreak_bad";
	
	public SystemHealthStatus( final FDSP_ConfigPathReq.Iface legacyConfig, ConfigurationApi configApi,
		Authorizer authorizer, AuthenticationToken token ) {
		
		this.legacyConfig = legacyConfig;
		this.configApi = configApi;
		this.authorizer = authorizer;
		this.token = token;
	}
	
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {
		
		List<FDSP_Node_Info_Type> services = legacyConfig.ListServices( new FDSP_MsgHdrType() );
		
        List<VolumeDescriptor> allVolumes = configApi.listVolumes("")
                .stream()
                .collect(Collectors.toList());
        
        List<VolumeDescriptor> filteredVolumes = configApi.listVolumes("")
                .stream()
                .filter(v -> authorizer.hasAccess(token, v.getName()))
                .collect(Collectors.toList());
		
        List<SystemHealth> statuses = new ArrayList<SystemHealth>();
        
        SystemHealth serviceHealth = getServiceStatus( services );
        SystemHealth capacityHealth = getCapacityStatus( allVolumes );
        SystemHealth firebreakHealth = getFirebreakStatus( filteredVolumes );
        
        statuses.add( serviceHealth );
        statuses.add( capacityHealth );
        statuses.add( firebreakHealth );
        
        HealthState overallState = getOverallState( serviceHealth.getState(), 
        	capacityHealth.getState(), firebreakHealth.getState() );
        
        JSONArray jsonArray = new JSONArray( statuses );
        JSONObject rtn = new JSONObject();
        
        rtn.put( "overall", overallState );
        rtn.put( "status", jsonArray );
        
		return new JsonResource( rtn );
	}
	
	private HealthState getOverallState( HealthState serviceState, HealthState capacityState, HealthState firebreakState ){
		
		 // figure out the overall state of the system
        HealthState overallHealth = HealthState.GOOD;
        
        // if services are bad - system is bad.
        if ( serviceState.equals( HealthState.BAD ) ){
        	overallHealth = HealthState.LIMITED;
        }
        // if capacity is bad, system is marginal
        else if ( capacityState.equals( HealthState.BAD ) ){
        	overallHealth = HealthState.MARGINAL;
        }
        
        // now that the immediate status are done, the rest is determined
        // by how many areas are in certain conditions.  We'll do it
        // by points.  okay = 1pt, bad = 2pts.  
        //
        // good is <= 1pts.  accetable <=3 marginal > 3
        int points = 0;
        points += getPointsForState( serviceState );
        points += getPointsForState( capacityState );
        points += getPointsForState( firebreakState );
        
        if ( points == 0 ){
        	overallHealth = HealthState.EXCELLENT;
        }
        else if ( points == 1 ){
        	overallHealth = HealthState.GOOD;
        }
        else if ( points <= 3 ){
        	overallHealth = HealthState.ACCEPTABLE;
        }
        else if ( points < 6 ){
        	overallHealth = HealthState.MARGINAL;
        }
        else {
        	overallHealth = HealthState.LIMITED;
        }
        
        return overallHealth;
	}
	
	/**
	 * Utility to get the points associated with a particular state
	 * 
	 * @param state
	 * @return
	 */
	private int getPointsForState( final HealthState state ){
		
		int points = 0;
		
		switch( state ){
			case OKAY:
				points = 1;
				break;
			case BAD:
				points = 2;
				break;
			default:
				break;
		}
		
		return points;
	}
	
	/**
	 * Generate a status object to rollup firebreak status for filtered volumes
	 * @param volDescs
	 * @return
	 */
	private SystemHealth getFirebreakStatus( List<VolumeDescriptor> volDescs ){
		
		SystemHealth status = new SystemHealth();
		status.setCategory( SystemHealth.CATEGORY.FIREBREAK );
		
		// convert descriptors into volume objects
		List<Context> volumes = convertVolDescriptors( volDescs );
		
		// query that stats to get raw capacity data
		MetricQueryCriteriaBuilder queryBuilder = new MetricQueryCriteriaBuilder();
		
		List<Metrics> metrics = Arrays.asList( 	Metrics.STC_SIGMA,
                                              	Metrics.LTC_SIGMA,
                                              	Metrics.STP_SIGMA,
												Metrics.LTP_SIGMA );
		
		DateRange range = new DateRange();
		range.setEnd( (new Date()).getTime() );
		
		MetricQueryCriteria query = queryBuilder.withContexts( volumes )
			.withSeriesTypes( metrics )
			.withRange( range )
			.build();
		
		final EntityManager em = SingletonRepositoryManager.instance().getMetricsRepository().newEntityManager();
		
        final List<VolumeDatapoint> queryResults = new MetricCriteriaQueryBuilder( em )
        	.searchFor( query )
        	.resultsList();
		
		try {
			
			FirebreakHelper fbh = new FirebreakHelper();
			List<Series> series = fbh.processFirebreak( queryResults );
			
			long volumesWithRecentFirebreak = series.stream()
				.filter( s -> {
					
					long fb = s.getDatapoints().stream()
						.filter( dp -> {
							
							// firebreak in the last 24hours
							if ( dp.getY() <= TimeUnit.DAYS.toSeconds( 1 ) ){
								return true;
							}
							
							return false;
						})
						.count();
					
					if ( fb > 0 ){
						return true;
					}
					
					return false;
				})
				.count();
			
			Double volumePercent = ((double)volumesWithRecentFirebreak / (double)volumes.size()) * 100;
			
			if ( volumePercent == 0 ){
				status.setState( HealthState.GOOD );
				status.setMessage( FIREBREAK_GOOD );
			}
			else if ( volumePercent <= 50 ){
				status.setState( HealthState.OKAY );
				status.setMessage( FIREBREAK_OKAY );
			}
			else {
				status.setState( HealthState.BAD );
				status.setMessage( FIREBREAK_BAD );
			}
			
		} catch (TException e) {
			
			status.setState( HealthState.UNKNOWN );
		}
		
		return status;
	}

	/**
	 * Generate a status object to rollup system capacity status
	 * @param volumes
	 * @return
	 */
	private SystemHealth getCapacityStatus( List<VolumeDescriptor> volDescs ){
		SystemHealth status = new SystemHealth();
		status.setCategory( SystemHealth.CATEGORY.CAPACITY );
		
		// convert descriptors into volume objects
		List<Context> volumes = convertVolDescriptors( volDescs );
		
		// query that stats to get raw capacity data
		MetricQueryCriteriaBuilder queryBuilder = new MetricQueryCriteriaBuilder();
	
		DateRange range = new DateRange();
		range.setEnd( (new Date()).getTime() );
		
		MetricQueryCriteria query = queryBuilder.withContexts( volumes )
			.withSeriesType( Metrics.PBYTES )
			.withRange( range )
			.build();
		
		final EntityManager em = SingletonRepositoryManager.instance().getMetricsRepository().newEntityManager();
		
        final List<VolumeDatapoint> queryResults = new MetricCriteriaQueryBuilder( em )
        	.searchFor( query )
        	.resultsList();
        
        // has some helper functions we can use for calculations
        QueryHelper qh = new QueryHelper();
        
        // TODO:  Replace this with the correct call to get real capacity
        final Double systemCapacity = Long.valueOf( SizeUnit.TB.totalBytes( 1 ) )
        	.doubleValue();
        
        final CapacityConsumed consumed = new CapacityConsumed();
        consumed.setTotal( SingletonRepositoryManager.instance()
        	.getMetricsRepository()
        	.sumPhysicalBytes() );
        
        List<Series> series = new SeriesHelper().getRollupSeries( queryResults, query, StatOperation.SUM );
        
        // use the helper to get the key metrics we'll use to ascertain the stat of our capacity
        CapacityFull capacityFull = qh.percentageFull( consumed, systemCapacity );
        CapacityToFull timeToFull = qh.toFull( series.get( 0 ), systemCapacity );
		
        Long daysToFull = TimeUnit.SECONDS.toDays( timeToFull.getToFull() );
        
        if ( daysToFull <= 7 ){
        	status.setState( HealthState.BAD );
        	status.setMessage( CAPACITY_BAD_RATE );
        }
        else if ( capacityFull.getPercentage() >= 90 ) {
        	status.setState( HealthState.BAD );
        	status.setMessage( CAPACITY_BAD_THRESHOLD );
        }
        else if ( daysToFull <= 30 ){
        	status.setState( HealthState.OKAY );
        	status.setMessage( CAPACITY_OKAY_RATE );
        }
        else if ( capacityFull.getPercentage() >= 80 ){
        	status.setState( HealthState.OKAY );
        	status.setMessage( CAPACITY_OKAY_THRESHOLD );
        }
        else {
        	status.setState( HealthState.GOOD );
        	status.setMessage( CAPACITY_GOOD );
        }
        
		return status;
	}
	
	/**
	 * Generate a status object to roll up service status
	 * @param services
	 * @return
	 */
	private SystemHealth getServiceStatus( final List<FDSP_Node_Info_Type> services ){
		
		SystemHealth status = new SystemHealth();
		status.setCategory( SystemHealth.CATEGORY.SERVICES );
		
		// first, if all the services are up, we're good.
		long servicesUp = services.stream()
			.filter( ( s ) -> {
				boolean up = s.getNode_state().equals( FDSP_NodeState.FDS_Node_Up );
				return up;
			})
			.count();
		
		// if every service we know about is up, then we're going to report good to go
		if ( servicesUp == services.size() ){
			status.setState( HealthState.GOOD );
			status.setMessage( SERVICES_GOOD );
			return status;
		}
		
		// No we will report okay if there is at least 1 om, 1 am in the system
		// and an sm,dm,pm running on each node.
		
		// first we do 2 groupings.  One into nodes, on into services
		Map<FDSP_MgrIdType, List<FDSP_Node_Info_Type>> byService = services.stream()
			.collect( Collectors.groupingBy( FDSP_Node_Info_Type::getNode_type ) );
		
		int nodes = services.stream()
			.collect( Collectors.groupingBy( FDSP_Node_Info_Type::getNode_uuid ) )
			.keySet()
			.size();
		
		// get a list of the services so we can use their counts
		List<FDSP_Node_Info_Type> oms = byService.get( FDSP_MgrIdType.FDSP_ORCH_MGR );
		List<FDSP_Node_Info_Type> ams = byService.get( FDSP_MgrIdType.FDSP_STOR_HVISOR );
		List<FDSP_Node_Info_Type> pms = byService.get( FDSP_MgrIdType.FDSP_PLATFORM );
		List<FDSP_Node_Info_Type> dms = byService.get( FDSP_MgrIdType.FDSP_DATA_MGR );
		List<FDSP_Node_Info_Type> sms = byService.get( FDSP_MgrIdType.FDSP_STOR_MGR );
		
		if ( oms != null && oms.size() > 0 &&
			ams != null && ams.size() > 0 && 
			pms != null && dms != null && sms != null &&
			pms.size() == nodes && sms.size() == nodes &&
			dms.size() == nodes ){
			status.setState( HealthState.OKAY );
			status.setMessage( SERVICES_OKAY );
		}
		else {
			status.setState( HealthState.BAD );
			status.setMessage( SERVICES_BAD );
		}
		
		return status;
	}
	
	/**
	 * Simple helper to convert a list of descriptors
	 * into actual volume objects
	 * 
	 * @param volDescs
	 * @return
	 */
	private List<Context> convertVolDescriptors( List<VolumeDescriptor> volDescs ){
		
		List<Context> volumes = volDescs.stream().map( ( vd ) -> {
			VolumeBuilder vBuilder = new VolumeBuilder();
			
			Volume volume = vBuilder.withId( Long.toString( vd.getVolId() ) )
				.withName( vd.getName() )
				.build();
			
			return volume;
		})
		.collect( Collectors.toList() );
		
		return volumes;
	}
	
}

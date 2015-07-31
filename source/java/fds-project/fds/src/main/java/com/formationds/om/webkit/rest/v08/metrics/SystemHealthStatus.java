/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.metrics;

import com.formationds.apis.VolumeDescriptor;
import com.formationds.client.v08.converters.ExternalModelConverter;
import com.formationds.client.v08.model.HealthState;
import com.formationds.client.v08.model.Node;
import com.formationds.client.v08.model.Service;
import com.formationds.client.v08.model.Service.ServiceState;
import com.formationds.client.v08.model.Size;
import com.formationds.client.v08.model.SizeUnit;
import com.formationds.client.v08.model.SystemHealth;
import com.formationds.client.v08.model.SystemStatus;
import com.formationds.client.v08.model.Volume;
import com.formationds.commons.model.DateRange;
import com.formationds.commons.model.Series;
import com.formationds.commons.model.calculated.capacity.CapacityConsumed;
import com.formationds.commons.model.calculated.capacity.CapacityFull;
import com.formationds.commons.model.calculated.capacity.CapacityToFull;
import com.formationds.commons.model.entity.IVolumeDatapoint;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.commons.model.type.Metrics;
import com.formationds.commons.model.type.StatOperation;
import com.formationds.om.repository.SingletonRepositoryManager;
import com.formationds.om.repository.helper.FirebreakHelper;
import com.formationds.om.repository.helper.QueryHelper;
import com.formationds.om.repository.helper.SeriesHelper;
import com.formationds.om.repository.query.MetricQueryCriteria;
import com.formationds.om.repository.query.builder.MetricQueryCriteriaBuilder;
import com.formationds.om.webkit.rest.v08.platform.ListNodes;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import org.apache.thrift.TException;
import org.eclipse.jetty.server.Request;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

public class SystemHealthStatus implements RequestHandler {

	private static final Logger logger = LoggerFactory.getLogger( SystemHealth.class );
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

    public enum CATEGORY{ CAPACITY, FIREBREAK, SERVICES };
    
    public SystemHealthStatus(ConfigurationApi configApi,
                              Authorizer authorizer, AuthenticationToken token) {

        this.configApi = configApi;
        this.authorizer = authorizer;
        this.token = token;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters)
            throws Exception {

    	logger.debug( "Retrieving the system health." );

        List<VolumeDescriptor> allVolumes = configApi.listVolumes( "" );
        List<VolumeDescriptor> filteredVolumes = allVolumes.stream()
                                                           .filter( v -> authorizer.ownsVolume( token, v.getName() ) )
                                                           .collect( Collectors.toList() );

        SystemHealth serviceHealth = getServiceStatus();
        SystemHealth capacityHealth = getCapacityStatus(allVolumes);
        SystemHealth firebreakHealth = getFirebreakStatus(filteredVolumes);

        HealthState overallState = getOverallState(serviceHealth.getState(),
                capacityHealth.getState(), firebreakHealth.getState());

        SystemStatus systemStatus = new SystemStatus();
        systemStatus.addStatus(serviceHealth);
        systemStatus.addStatus(capacityHealth);
        systemStatus.addStatus(firebreakHealth);
        systemStatus.setOverall(overallState);

        return new TextResource(ObjectModelHelper.toJSON(systemStatus));
    }

    private HealthState getOverallState(HealthState serviceState, HealthState capacityState, HealthState firebreakState) {

        // figure out the overall state of the system
        HealthState overallHealth = HealthState.GOOD;

        // if services are bad - system is bad.
        if (serviceState.equals(HealthState.BAD)) {
            overallHealth = HealthState.LIMITED;
        }
        // if capacity is bad, system is marginal
        else if (capacityState.equals(HealthState.BAD)) {
            overallHealth = HealthState.MARGINAL;
        }

        // now that the immediate status are done, the rest is determined
        // by how many areas are in certain conditions.  We'll do it
        // by points.  okay = 1pt, bad = 2pts.  
        //
        // good is <= 1pts.  accetable <=3 marginal > 3
        int points = 0;
        points += getPointsForState(serviceState);
        points += getPointsForState(capacityState);
        points += getPointsForState(firebreakState);

        if (points == 0) {
            overallHealth = HealthState.EXCELLENT;
        } else if (points == 1) {
            overallHealth = HealthState.GOOD;
        } else if (points <= 3) {
            overallHealth = HealthState.ACCEPTABLE;
        } else if (points < 6) {
            overallHealth = HealthState.MARGINAL;
        } else {
            overallHealth = HealthState.LIMITED;
        }
        
        logger.debug( "Overall health is: {}.", overallHealth.name() );

        return overallHealth;
    }

    /**
     * Utility to get the points associated with a particular state
     *
     * @param state
     * @return
     */
    private int getPointsForState(final HealthState state) {

        int points = 0;

        switch (state) {
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
     *
     * @param volDescs
     * @return
     */
    private SystemHealth getFirebreakStatus(List<VolumeDescriptor> volDescs) {

    	logger.debug( "Retrieving firebreak system status." );

        SystemHealth status = new SystemHealth();
        status.setCategory(CATEGORY.FIREBREAK.name());

        // convert descriptors into volume objects
        List<Volume> volumes = convertVolDescriptors(volDescs);

        // query that stats to get raw capacity data
        MetricQueryCriteriaBuilder queryBuilder = new MetricQueryCriteriaBuilder();

        List<Metrics> metrics = Arrays.asList(Metrics.STC_SIGMA,
                Metrics.LTC_SIGMA,
                Metrics.STP_SIGMA,
                Metrics.LTP_SIGMA);

        DateRange range = DateRange.last24Hours();

        MetricQueryCriteria query = queryBuilder.withContexts(volumes)
                .withSeriesTypes(metrics)
                .withRange(range)
                .build();

        final List<IVolumeDatapoint> queryResults = (List<IVolumeDatapoint>)SingletonRepositoryManager.instance()
                                                                                                      .getMetricsRepository()
                                                                                                      .query( query );

        try {

            FirebreakHelper fbh = new FirebreakHelper();
            List<Series> series = fbh.processFirebreak( queryResults );
            final Date now = new Date();

            long volumesWithRecentFirebreak = series.stream()
                    .filter(s -> {

                        long fb = s.getDatapoints().stream()
                                .filter(dp -> {

                                    // firebreak in the last 24hours
                                    if (dp.getY() > (now.getTime() - TimeUnit.DAYS.toSeconds(1))) {
                                        return true;
                                    }

                                    return false;
                                })
                                .count();

                        if (fb > 0) {
                            return true;
                        }

                        return false;
                    })
                    .count();

            Double volumePercent = ((double) volumesWithRecentFirebreak / (double) volumes.size()) * 100;

            if (volumesWithRecentFirebreak == 0 || volumePercent.isNaN() || volumePercent.isInfinite()) {
                status.setState(HealthState.GOOD);
                status.setMessage(FIREBREAK_GOOD);
            } else if (volumePercent <= 50) {
                status.setState(HealthState.OKAY);
                status.setMessage(FIREBREAK_OKAY);
            } else {
                status.setState(HealthState.BAD);
                status.setMessage(FIREBREAK_BAD);
            }

        } catch (TException e) {

            status.setState(HealthState.UNKNOWN);
        }

        logger.debug( "Firebreak status is: {}:{}.", status.getState().name(), status.getMessage() );
        
        return status;
    }

    /**
     * Generate a status object to rollup system capacity status
     */
    private SystemHealth getCapacityStatus(List<VolumeDescriptor> volDescs) {
    	logger.debug( "Getting system capacity status." );
    	
        SystemHealth status = new SystemHealth();
        status.setCategory(CATEGORY.CAPACITY.name());

        // convert descriptors into volume objects
        List<Volume> volumes = convertVolDescriptors(volDescs);

        // query that stats to get raw capacity data
        MetricQueryCriteriaBuilder queryBuilder = new MetricQueryCriteriaBuilder();

        DateRange range = DateRange.last24Hours();

        MetricQueryCriteria query = queryBuilder.withContexts(volumes)
                .withSeriesType(Metrics.PBYTES)
                .withRange(range)
                .build();

        final List<IVolumeDatapoint> queryResults = (List<IVolumeDatapoint>)SingletonRepositoryManager.instance()
                                                                                        .getMetricsRepository()
                                                                                        .query( query );

        // has some helper functions we can use for calculations
        QueryHelper qh = new QueryHelper();

        // TODO:  Replace this with the correct call to get real capacity
        final Double systemCapacity = Size.of( 1, SizeUnit.TB ).getValue( SizeUnit.B ).doubleValue();

        final CapacityConsumed consumed = new CapacityConsumed();
        consumed.setTotal(SingletonRepositoryManager.instance()
                .getMetricsRepository()
                .sumPhysicalBytes());

        List<Series> series = new SeriesHelper().getRollupSeries(queryResults, query, StatOperation.SUM);

        // use the helper to get the key metrics we'll use to ascertain the stat of our capacity
        CapacityFull capacityFull = qh.percentageFull(consumed, systemCapacity);
        CapacityToFull timeToFull = qh.toFull(series.get(0), systemCapacity);

        Long daysToFull = TimeUnit.SECONDS.toDays(timeToFull.getToFull());

        if (daysToFull <= 7) {
            status.setState(HealthState.BAD);
            status.setMessage(CAPACITY_BAD_RATE);
        } else if (capacityFull.getPercentage() >= 90) {
            status.setState(HealthState.BAD);
            status.setMessage(CAPACITY_BAD_THRESHOLD);
        } else if (daysToFull <= 30) {
            status.setState(HealthState.OKAY);
            status.setMessage(CAPACITY_OKAY_RATE);
        } else if (capacityFull.getPercentage() >= 80) {
            status.setState(HealthState.OKAY);
            status.setMessage(CAPACITY_OKAY_THRESHOLD);
        } else {
            status.setState(HealthState.GOOD);
            status.setMessage(CAPACITY_GOOD);
        }

        logger.debug( "Capacity status is: {}:{}.", status.getState().name(), status.getMessage() );
        
        return status;
    }

    /**
     * Generate a status object to roll up service status
     *
     * @return the system service status
     * @throws TException 
     */
    private SystemHealth getServiceStatus() throws TException {

    	logger.debug( "Getting the system service status." );
    	
        SystemHealth status = new SystemHealth();
        status.setCategory(CATEGORY.SERVICES.name());

        List<Node> nodes = (new ListNodes()).getNodes();
        
        List<Service> downServices = new ArrayList<>();
        Map<String, Boolean> criteria = new HashMap<String, Boolean>();
        
        String ONE_OM = "ONE_OM";
        String ONE_AM = "ONE_AM";
        String ALL_PMS = "ALL_PMS";
        String ALL_SMS = "ALL_SMS";
        String ALL_DMS = "ALL_DMS";
        
        criteria.put( ONE_AM, false );
        criteria.put( ONE_OM, false );
        criteria.put( ALL_DMS, true );
        criteria.put( ALL_SMS,  true );
        criteria.put( ALL_PMS, true );
        
        nodes.stream().forEach( (node) -> {
        	
        	node.getServices().keySet().stream().forEach( (serviceType) -> {
        		
        		node.getServices().get( serviceType ).stream().forEach( (service) -> {
        			
        			if ( !service.getStatus().getServiceState().equals( ServiceState.RUNNING ) ){
        				downServices.add( service );
        				
        				switch( service.getType() ){
        					case PM:
        						criteria.put( ALL_PMS, false );
        						break;
        					case SM:
        						criteria.put( ALL_SMS, false );
        						break;
        					case DM:
        						criteria.put( ALL_DMS, false );
        						break;
        					default:
        						break;
        				}
        			}
        			else {
        				
        				switch ( service.getType() ){
        					case OM:
        						criteria.put( ONE_OM, true );
        						break;
        					case AM: 
        						criteria.put( ONE_AM, true );
        					default:
        						break;
        				}
        			}
        		});
        	});
        });

        // if every service we know about is up, then we're going to report good to go
        if ( downServices.size() == 0 ) {
            status.setState(HealthState.GOOD);
            status.setMessage(SERVICES_GOOD);
        }
        else if ( criteria.get( ONE_AM ) &&
                  ( criteria.get( ONE_OM ) ) &&
                  ( criteria.get( ALL_DMS ) ) &&
                  ( criteria.get( ALL_PMS ) ) &&
                  ( criteria.get( ALL_SMS ) ) ){
        	status.setState(HealthState.OKAY);
        	status.setMessage(SERVICES_OKAY);
        }
        else {
        	status.setState(HealthState.BAD);
        	status.setMessage(SERVICES_BAD);
        }

        logger.debug( "Service status is: {}:{}.", status.getState().name(), status.getMessage() );
        
        return status;
    }

    /**
     * Simple helper to convert a list of descriptors
     * into actual volume objects
     *
     * @param volDescs
     * @return
     */
    private List<Volume> convertVolDescriptors(List<VolumeDescriptor> volDescs) {

        return ExternalModelConverter.convertToExternalVolumes( volDescs );

    }

}

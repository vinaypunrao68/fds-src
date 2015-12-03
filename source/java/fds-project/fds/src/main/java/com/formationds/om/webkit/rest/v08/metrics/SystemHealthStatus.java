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
import com.formationds.commons.togglz.feature.flag.FdsFeatureToggles;
import com.formationds.om.repository.MetricRepository;
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

    private static final String SERVICES_GOOD = "l_services_good";
    private static final String SERVICES_OKAY = "l_services_not_good";
    private static final String SERVICES_BAD  = "l_services_bad";

    private static final String CAPACITY_GOOD           = "l_capacity_good";
    private static final String CAPACITY_OKAY_THRESHOLD = "l_capacity_not_good_threshold";
    private static final String CAPACITY_OKAY_RATE      = "l_capacity_not_good_rate";
    private static final String CAPACITY_BAD_THRESHOLD  = "l_capacity_bad_threshold";
    private static final String CAPACITY_BAD_RATE       = "l_capacity_bad_rate";

    private static final String FIREBREAK_GOOD = "l_firebreak_good";
    private static final String FIREBREAK_OKAY = "l_firebreak_not_good";
    private static final String FIREBREAK_BAD  = "l_firebreak_bad";

    public enum CATEGORY {CAPACITY, FIREBREAK, SERVICES}
    
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

        List<VolumeDescriptor> allVolumeDescriptors = configApi.listVolumes( "" );

        // convert descriptors into volume objects
        List<Volume> allVolumes = convertVolDescriptors( allVolumeDescriptors );

        List<Volume> filteredVolumes = allVolumes.stream()
                                                 .filter( v -> authorizer.ownsVolume( token, v.getName() ) )
                                                 .collect( Collectors.toList() );

        SystemHealth serviceHealth = getServiceStatus();
        SystemHealth capacityHealth = getCapacityStatus(allVolumes);
        SystemHealth firebreakHealth = getFirebreakStatus(filteredVolumes);

        HealthState overallState = getOverallState( serviceHealth.getState(),
                                                    capacityHealth.getState(),
                                                    firebreakHealth.getState() );

        SystemStatus systemStatus = new SystemStatus();
        systemStatus.addStatus(serviceHealth);
        systemStatus.addStatus(capacityHealth);
        systemStatus.addStatus(firebreakHealth);
        systemStatus.setOverall( overallState );

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
     * @param state the state
     *
     * @return the number of points for the state
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

    private SystemHealth firebreakHealth( HealthState state ) {
        return new SystemHealth( CATEGORY.FIREBREAK.name(), state, firebreakMessage( state ) );
    }

    private String firebreakMessage( HealthState state ) {
        switch ( state ) {
            case GOOD:
                return FIREBREAK_GOOD;
            case OKAY:
                return FIREBREAK_OKAY;
            case BAD:
                return FIREBREAK_BAD;
            case UNKNOWN:
                return "";

            case ACCEPTABLE:
            case LIMITED:
            case MARGINAL:
            default:
                throw new IllegalArgumentException( "Unsupported health state for firebreak. " + state );
        }
    }
    /**
     * Generate a status object to rollup firebreak status for filtered volumes
     *
     * @param volumes the list of volumes to gather firebreak data on
     * @return the firebreak status
     */
    private SystemHealth getFirebreakStatus( List<Volume> volumes ) {

    	logger.debug( "Retrieving firebreak system status." );

        SystemHealth status;
        if ( volumes.size() == 0 ) {
            status = firebreakHealth( HealthState.GOOD );
            logger.debug( "Firebreak status is: {}:{}. (No active volumes)", status.getState().name(),
                          status.getMessage() );
            return status;
        }

        long volumesWithRecentFirebreak = 0;
        if ( !FdsFeatureToggles.FS_2660_METRIC_FIREBREAK_EVENT_QUERY.isActive() ) {

            // The Volume object already has a status from when loaded, so there is no need to
            // make additional queries to search for firebreak events.  Just count the columns that
            // have an active (last 24 hours) firebreak event.
            volumesWithRecentFirebreak = volumes.stream().filter( FirebreakHelper::hasActiveFirebreak ).count();

        } else {

            // query that stats to get raw capacity data
            MetricQueryCriteriaBuilder queryBuilder = new MetricQueryCriteriaBuilder();

            List<Metrics> metrics = Arrays.asList( Metrics.STC_SIGMA,
                                                   Metrics.LTC_SIGMA,
                                                   Metrics.STP_SIGMA,
                                                   Metrics.LTP_SIGMA );

            DateRange range = DateRange.last24Hours();
            MetricQueryCriteria query = queryBuilder.withContexts( volumes )
                                                    .withSeriesTypes( metrics )
                                                    .withRange( range )
                                                    .build();

            final MetricRepository metricsRepository = SingletonRepositoryManager.instance()
                                                                                 .getMetricsRepository();
            @SuppressWarnings("unchecked")
            final List<IVolumeDatapoint> queryResults = (List<IVolumeDatapoint>) metricsRepository.query( query );

            try {

                FirebreakHelper fbh = new FirebreakHelper();
                List<Series> series = fbh.processFirebreak( queryResults );
                final Date now = new Date();
                final long t24s = now.getTime() - TimeUnit.DAYS.toSeconds( 1 );

                // count firebreaks in the last 24hours
                volumesWithRecentFirebreak = series.stream()
                                                   .filter( s -> {
                                                       long fb = s.getDatapoints()
                                                                  .stream()
                                                                  .filter( dp -> (dp.getY() > t24s) )
                                                                  .count();

                                                       return (fb > 0);
                                                   } )
                                                   .count();
            } catch ( TException e ) {

                // this almost certainly means the server is down
                return firebreakHealth( HealthState.UNKNOWN );
            }
        }

        Double volumePercent = ((double) volumesWithRecentFirebreak / (double) volumes.size()) * 100;

        if ( volumesWithRecentFirebreak == 0 ) {
            status = firebreakHealth( HealthState.GOOD );
        } else if ( volumePercent <= 50 ) {
            status = firebreakHealth( HealthState.OKAY );
        } else {
            status = firebreakHealth( HealthState.BAD );
        }

        logger.debug( "Firebreak status is: {}:{}.", status.getState().name(), status.getMessage() );

        return status;
    }

    /**
     * Generate a status object to rollup system capacity status
     */
    private SystemHealth getCapacityStatus( List<Volume> volumes ) {
        logger.debug( "Getting system capacity status." );

        SystemHealth status = new SystemHealth();
        status.setCategory(CATEGORY.CAPACITY.name());

        // query that stats to get raw capacity data
        MetricQueryCriteriaBuilder queryBuilder = new MetricQueryCriteriaBuilder();

        // TODO: for capacity time-to-full we need enough history to calculate the regression
        // This was previously querying from 0 for all possible datapoints.  I think reducing to
        // the last 30 days is sufficient, but will need to validate that.
        DateRange range = DateRange.last( 30L, TimeUnit.DAYS );
        MetricQueryCriteria query = queryBuilder.withContexts(volumes)
                .withSeriesType(Metrics.PBYTES)
                .withRange(range)
                .build();

        final MetricRepository metricsRepository = SingletonRepositoryManager.instance()
                                                                             .getMetricsRepository();

        @SuppressWarnings("unchecked")
        final List<IVolumeDatapoint> queryResults = (List<IVolumeDatapoint>) metricsRepository.query( query );

        // has some helper functions we can use for calculations
        QueryHelper qh = new QueryHelper();

        Double systemCapacity = 0D;
        try {
        	systemCapacity = (double)configApi.getDiskCapacityTotal(); 
        } catch (TException te) {
        	throw new IllegalStateException("Failed to retrieve system capacity", te);
        }

        final CapacityConsumed consumed = new CapacityConsumed();
        consumed.setTotal( metricsRepository
                               .sumPhysicalBytes() );

        List<Series> series = new SeriesHelper().getRollupSeries( queryResults,
                                                                  query.getRange(),
                                                                  query.getSeriesType(),
                                                                  StatOperation.SUM );

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

                    if ( !service.getStatus().getServiceState().equals( ServiceState.RUNNING ) ) {

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
                    } else {

                        switch ( service.getType() ) {
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

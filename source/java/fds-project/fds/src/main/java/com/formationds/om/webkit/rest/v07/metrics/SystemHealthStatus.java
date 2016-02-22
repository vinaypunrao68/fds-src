package com.formationds.om.webkit.rest.v07.metrics;

import com.formationds.apis.VolumeDescriptor;
import com.formationds.client.v08.converters.ExternalModelConverter;
import com.formationds.client.v08.model.Volume;
import com.formationds.commons.model.DateRange;
import com.formationds.commons.model.Series;
import com.formationds.commons.model.Service;
import com.formationds.commons.model.SystemHealth;
import com.formationds.commons.model.SystemStatus;
import com.formationds.commons.model.calculated.capacity.CapacityConsumed;
import com.formationds.commons.model.calculated.capacity.CapacityFull;
import com.formationds.commons.model.calculated.capacity.CapacityToFull;
import com.formationds.commons.model.entity.IVolumeDatapoint;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.commons.model.type.HealthState;
import com.formationds.commons.model.type.ManagerType;
import com.formationds.commons.model.type.Metrics;
import com.formationds.commons.model.type.ServiceStatus;
import com.formationds.commons.model.type.ServiceType;
import com.formationds.commons.model.type.StatOperation;
import com.formationds.commons.togglz.feature.flag.FdsFeatureToggles;
import com.formationds.om.repository.MetricRepository;
import com.formationds.om.repository.SingletonRepositoryManager;
import com.formationds.om.repository.helper.FirebreakHelper;
import com.formationds.om.repository.helper.QueryHelper;
import com.formationds.om.repository.helper.SeriesHelper;
import com.formationds.om.repository.query.MetricQueryCriteria;
import com.formationds.om.repository.query.QueryCriteria.QueryType;
import com.formationds.om.repository.query.builder.MetricQueryCriteriaBuilder;
import com.formationds.protocol.svc.types.FDSP_NodeState;
import com.formationds.protocol.svc.types.FDSP_Node_Info_Type;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.formationds.util.SizeUnit;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import org.apache.thrift.TException;
import org.eclipse.jetty.server.Request;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

public class SystemHealthStatus implements RequestHandler {

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

    public SystemHealthStatus(ConfigurationApi configApi,
                              Authorizer authorizer, AuthenticationToken token) {

        this.configApi = configApi;
        this.authorizer = authorizer;
        this.token = token;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters)
            throws Exception {

        List<FDSP_Node_Info_Type> services = configApi.ListServices(0);

        List<VolumeDescriptor> allVolumeDescriptors = configApi.listVolumes( "" );
        List<Volume> allVolumes = ExternalModelConverter.convertToExternalVolumes( allVolumeDescriptors );
        List<Volume> filteredVolumes = allVolumes.stream()
                                                 .filter( v -> authorizer.ownsVolume( token, v.getName() ) )
                                                 .collect( Collectors.toList() );

        SystemHealth serviceHealth = getServiceStatus(services);
        SystemHealth capacityHealth = getCapacityStatus(allVolumes);
        SystemHealth firebreakHealth = getFirebreakStatus(filteredVolumes);

        HealthState overallState = getOverallState( serviceHealth.getState(),
                                                    capacityHealth.getState(),
                                                    firebreakHealth.getState() );

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

        return overallHealth;
    }

    /**
     * Utility to get the points associated with a particular state
     *
     * @param state the state
     * @return the points for the state
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
     * @param volumes the list of volumes
     * @return the firebreak status
     */
    private SystemHealth getFirebreakStatus( List<Volume> volumes ) {

        SystemHealth status = new SystemHealth();
        status.setCategory(SystemHealth.CATEGORY.FIREBREAK);

        long volumesWithRecentFirebreak = 0;
        if ( !FdsFeatureToggles.FS_2660_METRIC_FIREBREAK_EVENT_QUERY.isActive() ) {

            // The Volume object already has a status from when loaded, so there is no need to
            // make additional queries to search for firebreak events.  Just count the columns that
            // have an active (last 24 hours) firebreak event.
            volumesWithRecentFirebreak = volumes.stream().filter( FirebreakHelper::hasActiveFirebreak ).count();
        } else {

            // query that stats to get raw capacity data
            MetricQueryCriteriaBuilder queryBuilder = new MetricQueryCriteriaBuilder(QueryType.SYSHEALTH_FIREBREAK);

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

                // TODO: this almost certainly means the server is down
                status.setState( HealthState.UNKNOWN );
                status.setMessage( "Status query failed: " + e.getMessage() );
            }
        }

        Double volumePercent = ((double) volumesWithRecentFirebreak / (double) volumes.size()) * 100;

        if ( volumesWithRecentFirebreak == 0 || volumePercent.isNaN() || volumePercent.isInfinite() ) {
            status.setState( HealthState.GOOD );
            status.setMessage( FIREBREAK_GOOD );
        } else if ( volumePercent <= 50 ) {
            status.setState( HealthState.OKAY );
            status.setMessage( FIREBREAK_OKAY );
        } else {
            status.setState( HealthState.BAD );
            status.setMessage( FIREBREAK_BAD );
        }

        return status;
    }

    /**
     * Generate a status object to rollup system capacity status
     */
    private SystemHealth getCapacityStatus( List<Volume> volumes ) {
        SystemHealth status = new SystemHealth();
        status.setCategory(SystemHealth.CATEGORY.CAPACITY);

        // query that stats to get raw capacity data
        MetricQueryCriteriaBuilder queryBuilder = new MetricQueryCriteriaBuilder(QueryType.SYSHEALTH_CAPACITY);

        DateRange range = DateRange.last24Hours();

        MetricQueryCriteria query = queryBuilder.withContexts(volumes)
                .withSeriesType(Metrics.UBYTES)
                .withRange(range)
                .build();

        @SuppressWarnings("unchecked")
        final List<IVolumeDatapoint> queryResults = (List<IVolumeDatapoint>)SingletonRepositoryManager.instance()
                                                                                        .getMetricsRepository()
                                                                                        .query( query );

        // has some helper functions we can use for calculations
        QueryHelper qh = QueryHelper.instance();

        // TODO:  Replace this with the correct call to get real capacity
        final Double systemCapacity = Long.valueOf(SizeUnit.TB.totalBytes(1))
                .doubleValue();

        final CapacityConsumed consumed = new CapacityConsumed();
        consumed.setTotal(SingletonRepositoryManager.instance()
                .getMetricsRepository()
                .sumPhysicalBytes());

        List<Series> series = SeriesHelper.getRollupSeries( queryResults,
                                                                  query.getRange(),
                                                                  query.getSeriesType(),
                                                                  StatOperation.SUM );

        // use the helper to get the key metrics we'll use to ascertain the stat of our capacity
        CapacityFull capacityFull = qh.percentageFull(consumed, systemCapacity);
        CapacityToFull timeToFull = qh.toFull(series.get(0), systemCapacity);

        Long daysToFull = TimeUnit.SECONDS.toDays(timeToFull.getToFull());

        //noinspection Duplicates
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

        return status;
    }

    /**
     * Generate a status object to roll up service status
     *
     * @param rawServices
     * @return the system service status
     */
    private SystemHealth getServiceStatus(final List<FDSP_Node_Info_Type> rawServices) {

        SystemHealth status = new SystemHealth();
        status.setCategory(SystemHealth.CATEGORY.SERVICES);

        List<Service> services = new ArrayList<Service>();

        /**
         * We need to remove all services that are in the discovered state before we continue
         * because they do not inform the state of the system health.
         *
         * We are creating a new list here because we need the size to reflect the
         * reduced version of this list.
         */
        final List<FDSP_Node_Info_Type> filteredList = rawServices.stream()
        	.filter( (s) -> {

                if ( s.getNode_state().equals( FDSP_NodeState.FDS_Node_Discovered ) ){
	        		return false;
	        	}

                return true;
	        })
	        .collect( Collectors.toList() );

        // converting from the thrift type to our type
        filteredList.stream()
        	.forEach( service -> {
        		services.add( ServiceType.find( service ).get() );
        	});

        // first, if all the services are up, we're good.
        long servicesUp = services.stream()
                .filter((s) -> {
                    boolean up = s.getStatus().equals(ServiceStatus.ACTIVE);
                    return up;
                })
                .count();

        // if every service we know about is up, then we're going to report good to go
        if (servicesUp == services.size()) {
            status.setState(HealthState.GOOD);
            status.setMessage(SERVICES_GOOD);
            return status;
        }

        // No we will report okay if there is at least 1 om, 1 am in the system
        // and an sm,dm,pm running on each node.

        // first we do 2 groupings.  One into nodes, one into services
        Map<ManagerType, List<Service>> byService = services.stream()
                .collect(Collectors.groupingBy(Service::getType));

        int nodes = rawServices.stream()
                .collect(Collectors.groupingBy(FDSP_Node_Info_Type::getNode_uuid))
                .keySet()
                .size();

        // get a list of the services so we can use their counts
        List<Service> oms = byService.get(ManagerType.FDSP_ORCH_MGR);
        List<Service> ams = byService.get(ManagerType.FDSP_ACCESS_MGR);
        List<Service> pms = byService.get(ManagerType.FDSP_PLATFORM);
        List<Service> dms = byService.get(ManagerType.FDSP_DATA_MGR);
        List<Service> sms = byService.get(ManagerType.FDSP_STOR_MGR);

        if (oms != null && oms.size() > 0 &&
                ams != null && ams.size() > 0 &&
                pms != null && dms != null && sms != null &&
                pms.size() == nodes && sms.size() == nodes &&
                dms.size() == nodes) {
            status.setState(HealthState.OKAY);
            status.setMessage(SERVICES_OKAY);
        } else {
            status.setState(HealthState.BAD);
            status.setMessage(SERVICES_BAD);
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
    private List<Volume> convertVolDescriptors(List<VolumeDescriptor> volDescs) {

        List<Volume> volumes = volDescs.stream().map((vd) -> {

            Volume volume = new Volume( vd.getVolId(), vd.getName() );

            return volume;
        })
                .collect(Collectors.toList());

        return volumes;
    }

}

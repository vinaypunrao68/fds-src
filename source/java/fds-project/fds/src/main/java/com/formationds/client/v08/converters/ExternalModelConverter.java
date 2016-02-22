package com.formationds.client.v08.converters;

import com.formationds.apis.*;
import com.formationds.apis.VolumeType;
import com.formationds.client.ical.RecurrenceRule;
import com.formationds.client.v08.model.*;
import com.formationds.client.v08.model.Domain.DomainState;
import com.formationds.client.v08.model.MediaPolicy;
import com.formationds.client.v08.model.SnapshotPolicy;
import com.formationds.client.v08.model.SnapshotPolicy.SnapshotPolicyType;
import com.formationds.client.v08.model.Tenant;
import com.formationds.client.v08.model.User;
import com.formationds.client.v08.model.VolumeSettings;
import com.formationds.client.v08.model.VolumeStatus;
import com.formationds.client.v08.model.iscsi.Credentials;
import com.formationds.client.v08.model.iscsi.Initiator;
import com.formationds.client.v08.model.iscsi.LUN;
import com.formationds.client.v08.model.iscsi.Target;
import com.formationds.client.v08.model.nfs.NfsClients;
import com.formationds.client.v08.model.nfs.NfsOptions;
import com.formationds.commons.events.FirebreakType;
import com.formationds.commons.model.DateRange;
import com.formationds.commons.model.entity.Event;
import com.formationds.commons.model.entity.FirebreakEvent;
import com.formationds.commons.model.entity.IVolumeDatapoint;
import com.formationds.commons.model.type.Metrics;
import com.formationds.commons.togglz.feature.flag.FdsFeatureToggles;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.om.helper.SingletonConfiguration;
import com.formationds.om.redis.RedisSingleton;
import com.formationds.om.redis.VolumeDesc;
import com.formationds.om.repository.MetricRepository;
import com.formationds.om.repository.SingletonRepositoryManager;
import com.formationds.om.repository.helper.FirebreakHelper;
import com.formationds.om.repository.helper.FirebreakHelper.VolumeDatapointPair;
import com.formationds.om.repository.query.MetricQueryCriteria;
import com.formationds.om.repository.query.QueryCriteria.QueryType;
import com.formationds.om.util.AsyncAmClientFactory;
import com.formationds.om.util.AsyncAmMap;
import com.formationds.protocol.IScsiTarget;
import com.formationds.protocol.LogicalUnitNumber;
import com.formationds.protocol.NfsOption;
import com.formationds.protocol.svc.types.FDSP_MediaPolicy;
import com.formationds.protocol.svc.types.FDSP_VolType;
import com.formationds.protocol.svc.types.FDSP_VolumeDescType;
import com.formationds.protocol.svc.types.ResourceState;
import com.formationds.protocol.svc.types.SvcUuid;
import com.formationds.protocol.svc.types.VolumeGroupCoordinatorInfo;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.xdi.AsyncAm;
import org.apache.commons.lang3.StringUtils;
import org.apache.thrift.TException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.net.UnknownHostException;
import java.text.ParseException;
import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.EnumMap;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;
import java.util.stream.Collectors;

@SuppressWarnings("unused")
public class ExternalModelConverter {

    private static final String OBJECT_SIZE_NOT_SET =
        "Maximum object size was either not set, or outside the bounds of 4KB-8MB so it is being " +
        "set to the default of 2MB";
    private static final String BLOCK_SIZE_NOT_SET =
        "Block size was either not set, or outside the bounds of 4KB-8MB so it is being set to " +
        "the default of 128KB";

    private static ConfigurationApi configApi;
    private static final Integer DEF_BLOCK_SIZE  = (1024 * 128);
    private static final Integer DEF_OBJECT_SIZE = ((1024 * 1024) * 2);
    private static final Logger  logger          = LoggerFactory.getLogger( ExternalModelConverter.class );

    public static Domain convertToExternalDomain( LocalDomainDescriptor internalDomain ) {

        String extName = internalDomain.getName();
        Integer extId = internalDomain.getId();
        String site = internalDomain.getSite();

        Domain externalDomain = new Domain( extId, extName, site, DomainState.UP );

        return externalDomain;
    }

    public static LocalDomainDescriptor convertToInternalDomain( Domain externalDomain ) {

        LocalDomainDescriptor internalDomain = new LocalDomainDescriptor();

        internalDomain.setId( externalDomain.getId() );
        internalDomain.setName( externalDomain.getName() );
        internalDomain.setSite( externalDomain.getName() );

        return internalDomain;
    }

    public static User convertToExternalUser( com.formationds.apis.User internalUser ) {

        Long extId = internalUser.getId();
        String extName = internalUser.getIdentifier();
        Long roleId = 1L;

        logger.info( "Converting user ID: " + extId + " Name: " + extName );

        if ( internalUser.isIsFdsAdmin() ) {
            roleId = 0L;
        }

        Optional<com.formationds.apis.Tenant> tenant = getConfigApi().tenantFor( extId );

        Tenant externalTenant = null;

        if ( tenant.isPresent() ) {

            externalTenant = convertToExternalTenant( tenant.get() );
        }

        User externalUser = new User( extId, extName, roleId, externalTenant );

        return externalUser;
    }

    public static com.formationds.apis.User convertToInternalUser( User externalUser ) {

        com.formationds.apis.User internalUser = new com.formationds.apis.User();

        internalUser.setId( externalUser.getId() );
        internalUser.setIdentifier( externalUser.getName() );

        if ( externalUser.getRoleId() == 0L ) {
            internalUser.setIsFdsAdmin( true );
        } else {
            internalUser.setIsFdsAdmin( false );
        }

        return internalUser;
    }

    public static Tenant convertToExternalTenant( com.formationds.apis.Tenant internalTenant ) {

        Long id = internalTenant.getId();
        String name = internalTenant.getIdentifier();

        Tenant externalTenant = new Tenant( id, name );
        return externalTenant;
    }

    public static com.formationds.apis.Tenant convertToInternalTenant( Tenant externalTenant ) {

        com.formationds.apis.Tenant internalTenant = new com.formationds.apis.Tenant();
        internalTenant.setId( externalTenant.getId() );
        internalTenant.setIdentifier( externalTenant.getName() );

        return internalTenant;
    }

    public static MediaPolicy convertToExternalMediaPolicy( com.formationds.apis.MediaPolicy internalPolicy ) {

        MediaPolicy externalPolicy;

        switch ( internalPolicy ) {
            case HDD_ONLY:
                externalPolicy = MediaPolicy.HDD;
                break;
            case SSD_ONLY:
                externalPolicy = MediaPolicy.SSD;
                break;
            case HYBRID_ONLY:
            default:
                externalPolicy = MediaPolicy.HYBRID;
                break;
        }

        return externalPolicy;
    }

    public static com.formationds.apis.MediaPolicy convertToInternalMediaPolicy( MediaPolicy externalPolicy ) {

        com.formationds.apis.MediaPolicy internalPolicy;

        switch ( externalPolicy ) {

            case HDD:
                internalPolicy = com.formationds.apis.MediaPolicy.HDD_ONLY;
                break;
            case SSD:
                internalPolicy = com.formationds.apis.MediaPolicy.SSD_ONLY;
                break;
            case HYBRID:
            default:
                internalPolicy = com.formationds.apis.MediaPolicy.HYBRID_ONLY;
        }

        return internalPolicy;
    }

    public static VolumeState convertToExternalVolumeState( ResourceState internalState ) {

        VolumeState state = VolumeState.valueOf( internalState.name() );
        return state;
    }

    public static ResourceState convertToInternalVolumeState( VolumeState externalState ) {

        return ResourceState.valueOf( externalState.name() );
    }

    /**
     * Load the external volume status for each of the specified volumes
     *
     * @param volumes the list of volumes
     *
     * @return a map of volume id to the current volume status
     */
    public static Map<Long, VolumeStatus> loadExternalVolumeStatus( List<VolumeDescriptor> volumes ) {

        Map<Long, VolumeStatus> volumeStatus = new HashMap<>( volumes.size() * 2 );

        // Use a feature toggle so we can switch back to the old way of
        // querying firebreaks just in case the new way doesn't work in some case
        if ( FdsFeatureToggles.FS_2660_METRIC_FIREBREAK_EVENT_QUERY.isActive()) {

            // process the list of volume descriptors and get the external status values.
            // This should only be hitting cached data.
            volumes.stream().forEach( ( vd ) -> {
                Volume v = new Volume( vd.getVolId(), vd.getName() );
                EnumMap<FirebreakType, VolumeDatapointPair> vfb = getFirebreakEventsMetrics( v );
                VolumeStatus vstat = convertToExternalVolumeStatusMetric( vd, vfb );
                volumeStatus.put( vd.getVolId(), vstat );
            } );

        } else {
            // load all of the firebreak events in the last 24 hours across all volumes.
            Map<Long, EnumMap<FirebreakType, FirebreakEvent>> fbEvents = getFirebreakEvents();

            // process the list of volume descriptors and get the external status values.
            // This should only be hitting cached data.
            volumes.stream().forEach( ( vd ) -> {
                EnumMap<FirebreakType, FirebreakEvent> vfb = fbEvents.get( vd.getVolId() );
                VolumeStatus vstat = convertToExternalVolumeStatus( vd, vfb );
                volumeStatus.put( vd.getVolId(), vstat );
            } );
        }

        return volumeStatus;
    }

    private static VolumeStatus convertToExternalVolumeStatus( VolumeDescriptor internalVolume ) {
        Volume fakeVolume = new Volume( internalVolume.getVolId(), internalVolume.getName() );
        final EnumMap<FirebreakType, VolumeDatapointPair> fbResults = getFirebreakEventsMetrics( fakeVolume );
        return convertToExternalVolumeStatusMetric( internalVolume, fbResults );
    }

    private static VolumeStatus convertToExternalVolumeStatusMetric( VolumeDescriptor internalVolume,
                                                                     EnumMap<FirebreakType, VolumeDatapointPair> fbResults ) {
        return doConvertToExternalVolumeStatus( internalVolume, fbResults );
    }

    private static VolumeStatus convertToExternalVolumeStatus( VolumeDescriptor internalVolume,
                                                               EnumMap<FirebreakType, FirebreakEvent> fbResults ) {
        return doConvertToExternalVolumeStatus( internalVolume, fbResults );
    }

    /**
     * @param internalVolume the internal volume descriptor
     * @param fbResults      the firebreak results.  the value *must* be either a FirebreakEvent or a VolumeDatapointPair
     *
     * @return the volume status
     */
    private static VolumeStatus doConvertToExternalVolumeStatus( VolumeDescriptor internalVolume,
                                                                 EnumMap<FirebreakType, ?> fbResults ) {
        VolumeState volumeState = convertToExternalVolumeState( internalVolume.getState() );

        Optional<Size> extUsage = Optional.empty();

        FDSP_VolumeDescType volDescType = null;
        try
        {
            volDescType = getConfigApi().GetVolInfo( new FDSP_GetVolInfoReqType( internalVolume.getName( ),
                                                                                 0 /* Local Domain Id */ ) );
        }
        catch ( Exception e )
        {
            logger.warn( "The specified volume {}:{} encountered an error, reason {}.",
                         internalVolume.getName(),
                         internalVolume.getVolId(),
                         e.getMessage() != null ? e.getMessage() : "none reported" );
            logger.debug( "", e );
        }

        if( volDescType != null )
        {
            final Optional<VolumeGroupCoordinatorInfo> coordinatorInfo =
                Optional.ofNullable( volDescType.getCoordinator( ) );
            if( coordinatorInfo.isPresent( ) && ( coordinatorInfo.get( )
                                                                 .getId( )
                                                                 .getSvc_uuid( ) != 0 ) )
            {
                try
                {
                    logger.trace( "Requesting volume status for {}:{} from Volume group coordinator {}",
                                  internalVolume.getVolId(),
                                  internalVolume.getName(),
                                  coordinatorInfo.get( )
                                                 .getId( )
                                                 .getSvc_uuid( ) );

                    final AsyncAm am = AsyncAmMap.get( coordinatorInfo.get( )
                                                                      .getId( )
                                                                      .getSvc_uuid( ) );

                    CompletableFuture<com.formationds.apis.VolumeStatus> completableFuture =
                        am.volumeStatus( "" /* Local Domain Name */,
                                         internalVolume.getName() );

                    final com.formationds.apis.VolumeStatus vstatus = completableFuture.get();
                    if( vstatus != null )
                    {
                        extUsage =
                            Optional.of( Size.of( vstatus.getCurrentUsageInBytes( ), SizeUnit.B ) );

                    }
                }
                catch ( IOException | InterruptedException | ExecutionException e )
                {
                    logger.warn( "Failed to retrieve volume description for {}:{} - reason: {}.",
                                 internalVolume.getVolId(),
                                 internalVolume.getName(),
                                 e.getMessage() != null ? e.getMessage() : "none reported" );
                    logger.debug( "", e );

                    // force a new client to be created.
                    AsyncAmMap.failed( coordinatorInfo.get( )
                                                      .getId( )
                                                      .getSvc_uuid( ) );
                }
            }
            else
            {
                logger.trace( "Volume Coordinator isn't set for {}:{}, will attempt retrieving usage from stats",
                              internalVolume.getVolId(),
                              internalVolume.getName() );
            }
        }

        if( !extUsage.isPresent() )
        {
            final Optional<com.formationds.apis.VolumeStatus> optionalStatus =
                SingletonRepositoryManager.instance()
                                          .getMetricsRepository()
                                          .getLatestVolumeStatus( internalVolume.getName() );
            if( optionalStatus.isPresent() )
            {
                extUsage =
                    Optional.of( Size.of( optionalStatus.get().getCurrentUsageInBytes(),
                                          SizeUnit.B ) );
            }
        }

        if( !extUsage.isPresent() )
        {
            extUsage = Optional.of( Size.of( 0L, SizeUnit.B ) );
        }

        logger.trace( "Determined extUsage for " + internalVolume.getName() +
                      " to be " + extUsage + "." );

        Instant[] instants = {Instant.EPOCH, Instant.EPOCH};
        extractTimestamps( fbResults, instants );

        return new VolumeStatus( volumeState, extUsage.get(), instants[0], instants[1] );
    }

    /**
     * @param fbResults the firebreak results.  the value *must* be either a FirebreakEvent or a VolumeDatapointPair
     * @param instants  array of instants/timestamps that will be filled in
     */
    private static void extractTimestamps( EnumMap<FirebreakType, ?> fbResults, Instant[] instants ) {
        if ( fbResults != null ) {

            // put each firebreak event in the JSON object if it exists
            fbResults.forEach( ( type, event ) -> {

                Long ts = null;
                if ( event instanceof Event ) {
                    ts = ((Event) event).getInitialTimestamp();
                } else if ( event instanceof VolumeDatapointPair ) {
                    ts = ((VolumeDatapointPair) event).getDatapoint().getY().longValue();
                } else {
                    throw new IllegalStateException( "Unsupported type for firebreak results.  " +
                                                     "Expecting VolumeDatapoint or FirebreakEvent. Got " +
                                                     event.getClass() );
                }

                final Instant instant = Instant.ofEpochMilli( ts );
                if ( type.equals( FirebreakType.CAPACITY ) ) {
                    instants[0] = instant;
                } else if ( type.equals( FirebreakType.PERFORMANCE ) ) {
                    instants[1] = instant;
                }
            } );
        }
    }

    //		public static Snapshot convertToExternalSnapshot( com.formationds.apis.Snapshot internalSnapshot ){
    //
    //			long creation = internalSnapshot.getCreationTimestamp();
    //			long retentionInSeconds = internalSnapshot.getRetentionTimeSeconds();
    //			long snapshotId = internalSnapshot.getSnapshotId();
    //			String snapshotName = internalSnapshot.getSnapshotName();
    //			long volumeId = internalSnapshot.getVolumeId();
    //
    //			Snapshot externalSnapshot = new Snapshot( snapshotId,
    //					 								  snapshotName,
    //					 								  volumeId,
    //					 								  Duration.ofSeconds( retentionInSeconds ),
    //					 								  Instant.ofEpochMilli( creation ) );
    //
    //			return externalSnapshot;
    //		}

    public static com.formationds.protocol.svc.types.Snapshot convertToInternalSnapshot( Snapshot snapshot ) {

        com.formationds.protocol.svc.types.Snapshot internalSnapshot = new com.formationds.protocol.svc.types.Snapshot();

        internalSnapshot.setCreationTimestamp( snapshot.getCreationTime().toEpochMilli() );
        internalSnapshot.setRetentionTimeSeconds( snapshot.getRetention().getSeconds() );
        internalSnapshot.setSnapshotId( snapshot.getId() );
        internalSnapshot.setSnapshotName( snapshot.getName() );
        internalSnapshot.setVolumeId( snapshot.getVolumeId() );

        return internalSnapshot;
    }

    public static Snapshot convertToExternalSnapshot( com.formationds.protocol.svc.types.Snapshot protoSnapshot ) {

        long creation = protoSnapshot.getCreationTimestamp();
        long retentionInSeconds = protoSnapshot.getRetentionTimeSeconds();
        long volumeId = protoSnapshot.getVolumeId();
        String snapshotName = protoSnapshot.getSnapshotName();
        long snapshotId = protoSnapshot.getSnapshotId();

        return new Snapshot( snapshotId,
                             snapshotName,
                             volumeId,
                             Duration.ofSeconds( retentionInSeconds ),
                             Instant.ofEpochSecond( creation ) );
    }

    public static SnapshotPolicy convertToExternalSnapshotPolicy( com.formationds.apis.SnapshotPolicy internalPolicy ) {

        Long extId = internalPolicy.getId();
        String intRule = internalPolicy.getRecurrenceRule();
        Long intRetention = internalPolicy.getRetentionTimeSeconds();

        RecurrenceRule extRule = new RecurrenceRule();

        try {
            extRule = extRule.parser( intRule );
        } catch ( ParseException e ) {
            logger.warn( "Could not parse the retention rule: " + intRule );
            logger.debug( e.getMessage(), e );
        }

        Duration extRetention = Duration.ofSeconds( intRetention );

        SnapshotPolicyType type = SnapshotPolicyType.USER;

        if ( internalPolicy.getPolicyName().contains( SnapshotPolicyType.SYSTEM_TIMELINE.name() ) ) {
            type = SnapshotPolicyType.SYSTEM_TIMELINE;
        }

        return new SnapshotPolicy( extId, internalPolicy.getPolicyName(), type, extRule, extRetention );
    }

    public static com.formationds.apis.SnapshotPolicy convertToInternalSnapshotPolicy( SnapshotPolicy externalPolicy ) {
        com.formationds.apis.SnapshotPolicy internalPolicy = new com.formationds.apis.SnapshotPolicy();

        String policyName = externalPolicy.getName();
        internalPolicy.setId( externalPolicy.getId() );
        internalPolicy.setPolicyName( policyName );
        internalPolicy.setRecurrenceRule( externalPolicy.getRecurrenceRule().toString() );
        internalPolicy.setRetentionTimeSeconds( externalPolicy.getRetentionTime().getSeconds() );

        return internalPolicy;
    }

    /**
     * Extract the internal volume descriptor policy definitions to the external model representation
     * of data protection policies
     *
     * @param internalVolume
     * @param volDescType
     *
     * @return the external model data protection policy representation
     */
    protected static DataProtectionPolicy convertToExternalProtectionPolicy( VolumeDescriptor internalVolume, Optional<FDSP_VolumeDescType> volDescType  ) {

        Duration extCommitRetention = getCommitLogRetention( volDescType );

        // snapshot policies
        List<SnapshotPolicy> extPolicies = getExternalSnapshotPoliciesForVolume( internalVolume,
                                                                                 volDescType );
        return new DataProtectionPolicy( extCommitRetention, extPolicies );
    }

    /**
     *
     * @param internalVolume
     * @param volDescType an optional volume descriptor type. if present it indicates the volume exists.
     *
     * @return the list of snapshot policies for the volume, if it exists as indicated by presence of the volDescType.
     */
    static List<SnapshotPolicy> getExternalSnapshotPoliciesForVolume( VolumeDescriptor internalVolume,
                                                                      Optional<FDSP_VolumeDescType> volDescType) {
        List<SnapshotPolicy> extPolicies = new ArrayList<>();
        if (volDescType.isPresent()) {
            try {
                final List<com.formationds.apis.SnapshotPolicy> internalPolicies =
                    getConfigApi().listSnapshotPoliciesForVolume( internalVolume.getVolId() );

                for ( com.formationds.apis.SnapshotPolicy internalPolicy : internalPolicies ) {

                    SnapshotPolicy externalPolicy = convertToExternalSnapshotPolicy( internalPolicy );
                    extPolicies.add( externalPolicy );
                }
            } catch ( TException e ) {

                logger.warn( "Error occurred while trying to retrieve snapshot policies for volume: " +
                             internalVolume.getVolId() );
                logger.debug( "Exception: ", e );
            }
        }
        return extPolicies;
    }

    /**
     * @param internalVolume
     * @return the volume description type if the volume exists, Optional.empty if the volume does not exist or cannot be retrieved.
     */
    static Optional<FDSP_VolumeDescType> getVolumeInfo( VolumeDescriptor internalVolume ) {

        if ( internalVolume.getVolId() != 0 ) {
            try {
                FDSP_VolumeDescType volDescType =
                        getConfigApi().GetVolInfo( new FDSP_GetVolInfoReqType( internalVolume.getName(), 0 ) );
                return Optional.of(volDescType);
            } catch ( TException te ) {
                // typically volume not found/deleted
                logger.warn( "Error occurred while trying to retrieve volume: {}({}): {}", internalVolume.getName(), internalVolume.getVolId(), te.getMessage());
                logger.debug( "Exception: ", te );
                return Optional.empty();
            }
        } else {
            return Optional.empty();
        }
    }

    /**
     * @param volDescType
     *
     * @return if the volume descriptor is present, return its commit log retention period.  Otherwise return
     * the default commit log retention of 1 day
     */
    static Duration getCommitLogRetention(Optional<FDSP_VolumeDescType> volDescType) {
        if (volDescType.isPresent()) {
            long clRetention = volDescType.get().getContCommitlogRetention();
            return Duration.ofSeconds( clRetention );
        } else {
            return Duration.ofDays( 1 );
        }
    }

    /**
     * Given a list of <i>internal</i> Volume Descriptors, convert to a list of <i>external</i>
     * volume model objects
     *
     * @param internalVolumes the list of volume descriptors
     *
     * @return the list of external volume model objects
     */
    public static List<Volume> convertToExternalVolumes( List<VolumeDescriptor> internalVolumes ) {
        List<Volume> ext = new ArrayList<>();

        Map<Long, VolumeStatus> volumeStatus = loadExternalVolumeStatus( internalVolumes );

        // this is the naive way we are trying to get rid of
        internalVolumes.stream().forEach( descriptor -> {
            if( descriptor != null )
            {
                logger.trace(
                    "Converting volume " + descriptor.getName( ) + " to external format." );
                ext.add( ExternalModelConverter.convertToExternalVolume(
                    descriptor,
                    volumeStatus.get( descriptor.getVolId( ) ) ) );
            }
        } );

        return ext;
    }

    /**
     * Convert the internal VolumeDescriptor to the external v08 Volume representation.
     *
     * If the volume descriptor's volume id is zero (undefined), we assume that this is
     * a conversion for the purposes of creating a new volume and will skip filling in
     * certain pieces of information including current state.
     *
     * @param internalVolume
     * @param extStatus
     *
     * @return
     */
    public static Volume convertToExternalVolume( VolumeDescriptor internalVolume, VolumeStatus extStatus ) {

        String extName = internalVolume.getName();
        Long volumeId = internalVolume.getVolId();

        // if the volume id is null or 0, then lets assume this is a conversion in
        // preparation for volume creation (rather than mistake).  We could also use
        // the extStatus for this (i.e extStatus == null || extStatus.getState() == unknown
        boolean create = volumeId == null || volumeId == 0;

        com.formationds.apis.VolumeSettings settings = internalVolume.getPolicy();
        MediaPolicy extPolicy = convertToExternalMediaPolicy( settings.getMediaPolicy() );

        // makes external call to redis for nfs/iscsi settings.
        Optional<VolumeDesc> redisVolumeDesc = RedisSingleton.INSTANCE.api( ).getVolume( volumeId );

        // get the internal volume descriptor type if available (i.e. existing volume)
        // that is used for getting the policies associated with the volume
        Optional<FDSP_VolumeDescType> volDescType = getVolumeInfo( internalVolume );

        VolumeSettings extSettings = convertToExternalVolumeSettings( internalVolume,
                                                                      settings,
                                                                      redisVolumeDesc,
                                                                      extName,
                                                                      volumeId );

        QosPolicy extQosPolicy = getExternalQosPolicyForVolume( volDescType );
        Optional<Tenant> extTenant = findExternalTenantForVolume( internalVolume );
        VolumeAccessPolicy extAccessPolicy = VolumeAccessPolicy.exclusiveRWPolicy();
        DataProtectionPolicy extProtectionPolicy = convertToExternalProtectionPolicy( internalVolume,
                                                                                      volDescType );

        Instant extCreation = Instant.ofEpochMilli( internalVolume.getDateCreated() );

        return new Volume( volumeId,
                           extName,
                           extTenant.orElse( null ),
                           "Application",
                           extStatus,
                           extSettings,
                           extPolicy,
                           extProtectionPolicy,
                           extAccessPolicy,
                           extQosPolicy,
                           extCreation,
                           null );
    }

    /**
     * @param volDescType
     * @return
     */
    protected static QosPolicy getExternalQosPolicyForVolume( Optional<FDSP_VolumeDescType> volDescType ) {

        if (volDescType.isPresent()) {
            FDSP_VolumeDescType vdt = volDescType.get();
            QosPolicy extQosPolicy = new QosPolicy( vdt.getRel_prio(),
                                                    (int) vdt.getIops_assured(),
                                                    (int) vdt.getIops_throttle() );
            return extQosPolicy;
        } else {
            return QosPolicy.defaultPolicy();
        }
    }

    /**
     * For the internal volume descriptor find the internal tenant and convert it to the
     * external model tenant representation.
     *
     * @param internalVolume
     *
     * @return an optional containing the external tenant representation for the tenant the
     * volume is assigned to, or Optional.empty if not found
     */
    static Optional<Tenant> findExternalTenantForVolume( VolumeDescriptor internalVolume ) {

        try {
            List<com.formationds.apis.Tenant> internalTenants = getConfigApi().listTenants( 0 );

            for ( com.formationds.apis.Tenant internalTenant : internalTenants ) {

                if ( internalTenant.getId() == internalVolume.getTenantId() ) {
                    return Optional.of( convertToExternalTenant( internalTenant ) );
                }
            }
        } catch ( TException e ) {

            logger.warn( "Error occurred while trying to retrieve a list of tenants." );
            logger.debug( "Exception: ", e );
        }

        return Optional.empty();
    }
    /**
     * @param internalVolume
     * @param settings
     * @param extName
     * @param volumeId
     * @return
     */
    protected static VolumeSettings convertToExternalVolumeSettings( VolumeDescriptor internalVolume,
                                                                     com.formationds.apis.VolumeSettings settings,
                                                                     Optional<VolumeDesc> redisVolume,
                                                                     String extName,
                                                                     Long volumeId ) {
        VolumeSettings extSettings = null;

        if ( settings.getVolumeType().equals( VolumeType.BLOCK ) ) {

            Size capacity = Size.of( settings.getBlockDeviceSizeInBytes(), SizeUnit.B );
            Size blockSize = Size.of( settings.getMaxObjectSizeInBytes(), SizeUnit.B );

            extSettings = new VolumeSettingsBlock( capacity, blockSize );
        } else if ( settings.getVolumeType().equals( VolumeType.ISCSI ) ) {
            IScsiTarget iscsi = null;
            if( internalVolume.getPolicy().getIscsiTarget() == null ||
                internalVolume.getPolicy().getIscsiTarget().isSetLuns() )
            {
                if( redisVolume.isPresent( ) )
                {
                    final VolumeDesc desc = redisVolume.get( );
                    if( desc.settings( ).getIscsiTarget() != null &&
                        desc.settings( ).getIscsiTarget().isSetLuns( ) )
                    {
                        iscsi = desc.settings().getIscsiTarget();
                    }
                }
            }
            else
            {
                iscsi = internalVolume.getPolicy( ).getIscsiTarget();
            }

            if( iscsi != null )
            {
                Size capacity = Size.of( settings.getBlockDeviceSizeInBytes(), SizeUnit.B );
                Size blockSize = Size.of( settings.getMaxObjectSizeInBytes(), SizeUnit.B );

                extSettings = new VolumeSettingsISCSI.Builder( )
                    .withCapacity( capacity )
                    .withBlockSize( blockSize )
                    .withUnit( SizeUnit.B )
                    .withTarget(
                        new Target.Builder( )
                            .withLuns( convertToExternalLUN( iscsi.getLuns( ) ) )
                            .withInitiators( convertToExternalInitiators( iscsi.getInitiators() ) )
                            .withIncomingUsers( convertToExternalIncomingUser( iscsi.getIncomingUsers( ) ) )
                            .withOutgoingUsers( convertToExternalOutgoingUser( iscsi.getOutgoingUsers( ) ) )
                            .build( ) )
                    .build( );
            }
            else
            {
                logger.warn( "The iSCSI volume is missing mandatory attributes, skipping volume {}:{}",
                             extName,
                             volumeId );
            }

        }
        else if ( settings.getVolumeType().equals( VolumeType.NFS ) )
        {
            logger.trace( "NFS::SETTINGS::{}", internalVolume.getPolicy( ) );
            NfsOption nfsOptions = null;
            if ( !internalVolume.getPolicy( )
                                .isSetNfsOptions( ) )
            {
                if( redisVolume.isPresent( ) )
                {
                    final VolumeDesc desc = redisVolume.get( );
                    if( desc.settings( ).getNfsOptions() != null )
                    {
                        nfsOptions = desc.settings().getNfsOptions( );
                    }
                }
            }
            else
            {
                nfsOptions = internalVolume.getPolicy( )
                                           .getNfsOptions( );
            }

            if( nfsOptions != null )
            {
                final Size maxObjectSize = Size.of( settings.getMaxObjectSizeInBytes( ), SizeUnit.B );
                final Size capacity = Size.of( settings.getBlockDeviceSizeInBytes(), SizeUnit.B );

                try
                {
                    extSettings = new VolumeSettingsNfs.Builder()
                        .withOptions( convertToExternalNfsOptions( nfsOptions ) )
                        .withClient( convertToExternalNfsClients( nfsOptions ) )
                        .withMaxObjectSize( Size.of( settings.getMaxObjectSizeInBytes(), SizeUnit.B ) )
                        .withCapacity( capacity )
                        .build();
                }
                catch ( UnknownHostException e )
                {
                    logger.warn( "The NFS volume conversion failed {}, skipping volume {}:{}",
                                 e.getMessage(),
                                 extName,
                                 volumeId );
                    logger.debug( e.getMessage(), e );
                }
            }
            else
            {
                logger.warn( "The NFS volume is missing mandatory attributes, skipping volume {}:{}",
                             extName,
                             volumeId );
            }
        } else if ( settings.getVolumeType().equals( VolumeType.OBJECT ) ) {
            Size maxObjectSize = Size.of( settings.getMaxObjectSizeInBytes(), SizeUnit.B );

            extSettings = new VolumeSettingsObject( maxObjectSize );
        }
        return extSettings;
    }

    public static NfsOption convertToInternalNfsOptions( final String nfs, final String client )
    {
        if( nfs == null )
        {
            return new NfsOption( );
        }

        final NfsOption options = new NfsOption( );
        options.setOptions( nfs );
        options.setClient( client );

        return options;
    }

    public static NfsOptions convertToExternalNfsOptions( final NfsOption options )
    {
        final List<String> listOfOptions =
            Arrays.asList( StringUtils.split( options.getOptions(), "," ) );

        NfsOptions.Builder builder = new NfsOptions.Builder( );
        for( final String option : listOfOptions )
        {
            final String lowerCaseVersion = option.toLowerCase();
            logger.trace( "NFS Option [ '{}' ]", lowerCaseVersion );
            switch( lowerCaseVersion )
            {
                case "ro":
                    logger.trace( "NFS Option Builder {}", lowerCaseVersion );
                    builder = builder.ro( );
                    break;
                case "rw":
                    logger.trace( "NFS Option Builder {}", lowerCaseVersion );
                    builder = builder.rw( );
                    break;
                case "acl":
                    logger.trace( "NFS Option Builder {}", lowerCaseVersion );
                    builder = builder.withAcl( );
                    break;
                case "async":
                    logger.trace( "NFS Option Builder {}", lowerCaseVersion );
                    builder = builder.async( );
                    break;
                case "root_squash":
                    logger.trace( "NFS Option Builder {}", lowerCaseVersion );
                    builder = builder.withRootSquash( );
                case "all_squash":
                    logger.trace( "NFS Option Builder {}", lowerCaseVersion );
                    builder = builder.allSquash( );
                    break;
                default:
                    logger.trace( "NFS Option Builder {}", lowerCaseVersion );
                    break;
            }
        }
        final NfsOptions nfsOptions = builder.build();
        logger.trace( "Internal NFS Options::{} External NFS Options::{}", options, nfsOptions );
        return nfsOptions;
    }

    public static NfsClients convertToExternalNfsClients( final NfsOption options )
        throws UnknownHostException
    {
        return new NfsClients( options.getClient() );
    }

    public static List<LogicalUnitNumber> convertToInternalLogicalUnitNumber( final List<LUN> luns )
    {
        if( luns == null )
        {
            return new ArrayList<>( );
        }

        final List<LogicalUnitNumber> intLuns = new ArrayList<>( );
        for ( final LUN lun : luns )
        {
            intLuns.add( new LogicalUnitNumber( lun.getLunName( ),
                                                lun.getAccessType( )
                                                   .name( ) ) );
        }

        return intLuns;
    }

    public static List<LUN> convertToExternalLUN( final List<LogicalUnitNumber> logicalUnitNumbers )
    {
        if( logicalUnitNumbers == null )
        {
            return new ArrayList<>( );
        }

        return logicalUnitNumbers.stream( )
                                 .map( logicalUnitNumber -> new LUN.Builder( )
                                     .withLun( logicalUnitNumber.getName( ) )
                                     .withAccessType( LUN.AccessType
                                                         .valueOf( logicalUnitNumber.getAccess( ) ) )
                                     .build( ) )
                                 .collect( Collectors.toList( ) );
    }

    public static List<com.formationds.protocol.Initiator> convertToInternalInitiators(
        final List<Initiator> initiators )
    {
        if( initiators == null )
        {
            return new ArrayList<>( );
        }

        return initiators.stream( )
                         .map( initiator -> new com.formationds.protocol.Initiator( initiator.getWWNMask( ) ) )
                         .collect( Collectors.toList( ) );
    }

    public static List<Initiator> convertToExternalInitiators( final List<com.formationds.protocol.Initiator> initiators )
    {
        if( initiators == null )
        {
            return new ArrayList<>( );
        }

        return initiators.stream( )
                                 .map( initiator -> new Initiator( initiator.getWwn_mask() ) )
                                 .collect( Collectors.toList( ) );
    }

    public static List<com.formationds.protocol.Credentials> convertToInternalIncomingUsers(
        final List<Credentials> incomingUsers )
    {
        if( incomingUsers == null )
        {
            return new ArrayList<>( );
        }
        return incomingUsers.stream( )
                            .filter( ( user ) -> user.getUsername() != null && user.getUsername().length() > 0 )
                            .filter( ( user ) -> user.getPassword() != null && user.getPassword().length() > 0 )
                         .map( user -> new com.formationds.protocol.Credentials( user.getUsername(),
                                                                                 user.getPassword() ) )
                         .collect( Collectors.toList( ) );
    }

    public static List<Credentials> convertToExternalIncomingUser( final List<com.formationds.protocol.Credentials> incomingUsers )
    {
        if( incomingUsers == null )
        {
            return new ArrayList<>( );
        }

        return incomingUsers.stream( )
                            .filter( ( user ) -> user.getName() != null && user.getName().length() > 0 )
                            .filter( ( user ) -> user.getPasswd() != null && user.getPasswd().length() > 0 )
                         .map( user -> new Credentials( user.getName(), user.getPasswd() ) )
                         .collect( Collectors.toList( ) );
    }

    public static List<com.formationds.protocol.Credentials> convertToInternalOutgoingUsers(
        final List<Credentials> outgoingUsers )
    {
        if( outgoingUsers == null )
        {
            return new ArrayList<>( );
        }

        return outgoingUsers.stream( )
                            .filter( ( user ) -> user.getUsername() != null && user.getUsername().length() > 0 )
                            .filter( ( user ) -> user.getPassword() != null && user.getPassword().length() > 0 )
                            .map( user -> new com.formationds.protocol.Credentials( user.getUsername(),
                                                                                    user.getPassword() ) )
                            .collect( Collectors.toList( ) );
    }

    public static List<Credentials> convertToExternalOutgoingUser( final List<com.formationds.protocol.Credentials> outgoingUsers )
    {
        if( outgoingUsers == null )
        {
            return new ArrayList<>( );
        }

        return outgoingUsers.stream( )
                            .filter( ( user ) -> user.getName() != null && user.getName().length() > 0 )
                            .filter( ( user ) -> user.getPasswd() != null && user.getPasswd().length() > 0 )
                            .map( user -> new Credentials( user.getName(), user.getPasswd() ) )
                            .collect( Collectors.toList( ) );
    }

    public static VolumeDescriptor convertToInternalVolumeDescriptor( Volume externalVolume ) {

        VolumeDescriptor internalDescriptor = new VolumeDescriptor();

        if ( externalVolume.getCreated() != null ) {
            internalDescriptor.setDateCreated( externalVolume.getCreated().toEpochMilli() );
        }

        internalDescriptor.setName( externalVolume.getName() );

        if ( externalVolume.getStatus() != null && externalVolume.getStatus().getState() != null ) {
            internalDescriptor.setState( convertToInternalVolumeState( externalVolume.getStatus().getState() ) );
        }

        if ( externalVolume.getTenant() != null ) {
            internalDescriptor.setTenantId( externalVolume.getTenant().getId() );
        }

        if ( externalVolume.getId() != null ) {
            internalDescriptor.setVolId( externalVolume.getId() );
        }

        com.formationds.apis.VolumeSettings internalSettings =
                extractInternalVolumeSettings( externalVolume );

        internalDescriptor.setPolicy( internalSettings );

        return internalDescriptor;
    }

    /**
     * Extract and convert the internal volume settings from the external volume representation.
     *
     * @param externalVolume
     *
     * @return the internal volume settings
     */
    protected static com.formationds.apis.VolumeSettings extractInternalVolumeSettings( Volume externalVolume ) {

        com.formationds.apis.VolumeSettings internalSettings = new com.formationds.apis.VolumeSettings();
        if ( externalVolume.getSettings() instanceof VolumeSettingsISCSI )
        { // iSCSI volume
            VolumeSettingsISCSI iscsiSettings = ( VolumeSettingsISCSI ) externalVolume.getSettings( );

            if( iscsiSettings.getCapacity() == null )
            {
                throw new IllegalArgumentException( "iSCSI capacity must be provided" );
            }

            internalSettings.setBlockDeviceSizeInBytes( iscsiSettings.getCapacity( )
                                                                     .getValue( SizeUnit.B )
                                                                     .longValue( ) );

            Size blockSize = iscsiSettings.getBlockSize( );
            if ( blockSize != null &&
                blockSize.getValue( SizeUnit.B ).longValue( ) >= Size.of( 4, SizeUnit.KB )
                                                                     .getValue( SizeUnit.B )
                                                                     .longValue( ) &&
                blockSize.getValue( SizeUnit.B ).longValue( ) <= Size.of( 8, SizeUnit.MB )
                                                                     .getValue( SizeUnit.B )
                                                                     .longValue( ) )
            {
                internalSettings.setMaxObjectSizeInBytes( blockSize.getValue( SizeUnit.B )
                                                                   .intValue( ) );
            }
            else
            {
                logger.warn( BLOCK_SIZE_NOT_SET );
                internalSettings.setMaxObjectSizeInBytes( DEF_BLOCK_SIZE );
            }

            internalSettings.setVolumeType( VolumeType.ISCSI );
            if( iscsiSettings.getTarget() == null )
            {
                throw new IllegalArgumentException(
                    String.format( "The iSCSI volume is missing mandatory target, skipping volume %s:%s",
                                   externalVolume.getName(),
                                   externalVolume.getId() ) );
            }
            else
            {
                if( iscsiSettings.getTarget().getLuns().isEmpty() )
                {
                    iscsiSettings.getTarget()
                                 .getLuns()
                                 .add( new LUN.Builder()
                                              .withLun( externalVolume.getName() )
                                              .withAccessType( LUN.AccessType.RW )
                                              .build() );
                }

                final IScsiTarget iscsiTarget =
                    new IScsiTarget( ).setLuns(
                        convertToInternalLogicalUnitNumber(
                            iscsiSettings.getTarget( )
                                         .getLuns( ) ) )
                                      .setInitiators(
                                          convertToInternalInitiators(
                                              iscsiSettings.getTarget().getInitiators() ) )
                                      .setIncomingUsers(
                                          convertToInternalIncomingUsers(
                                              iscsiSettings.getTarget( ).getIncomingUsers( ) ) )
                                      .setOutgoingUsers(
                                          convertToInternalOutgoingUsers(
                                              iscsiSettings.getTarget( ).getOutgoingUsers( ) ) );

                internalSettings.setIscsiTarget( iscsiTarget );
            }
        }
        else if ( externalVolume.getSettings() instanceof VolumeSettingsBlock ) {        // block volume

            VolumeSettingsBlock blockSettings = ( VolumeSettingsBlock ) externalVolume.getSettings( );

            internalSettings
                .setBlockDeviceSizeInBytes( blockSettings.getCapacity( )
                                                         .getValue( SizeUnit.B )
                                                         .longValue( ) );

            Size blockSize = blockSettings.getBlockSize( );

            if ( blockSize != null &&
                blockSize.getValue( SizeUnit.B )
                         .longValue( ) >= Size.of( 4, SizeUnit.KB )
                                              .getValue( SizeUnit.B )
                                              .longValue( ) &&
                blockSize.getValue( SizeUnit.B )
                         .longValue( ) <= Size.of( 8, SizeUnit.MB )
                                              .getValue( SizeUnit.B )
                                              .longValue( ) )
            {
                internalSettings.setMaxObjectSizeInBytes( blockSize.getValue( SizeUnit.B )
                                                                   .intValue( ) );
            } else
            {
                logger.warn( BLOCK_SIZE_NOT_SET );
                internalSettings.setMaxObjectSizeInBytes( DEF_BLOCK_SIZE );
            }

            internalSettings.setVolumeType( VolumeType.BLOCK );
        }
        else if ( externalVolume.getSettings() instanceof VolumeSettingsNfs )
        {   // NFS volume
            VolumeSettingsNfs nfsSettings = ( VolumeSettingsNfs ) externalVolume.getSettings( );

            Size maxObjSize = nfsSettings.getMaxObjectSize();

            internalSettings
                .setBlockDeviceSizeInBytes( nfsSettings.getCapacity( )
                                                         .getValue( SizeUnit.B )
                                                         .longValue( ) );

            if ( maxObjSize != null &&
                 maxObjSize.getValue( SizeUnit.B ).longValue() >= Size.of( 4, SizeUnit.KB )
                                                                      .getValue( SizeUnit.B )
                                                                      .longValue() &&
                 maxObjSize.getValue( SizeUnit.B ).longValue() <= Size.of( 8, SizeUnit.MB )
                                                                      .getValue( SizeUnit.B )
                                                                      .longValue() )
            {
                internalSettings.setMaxObjectSizeInBytes( maxObjSize.getValue( SizeUnit.B )
                                                                    .intValue() );
            }
            else
            {
                logger.warn( OBJECT_SIZE_NOT_SET );
                internalSettings.setMaxObjectSizeInBytes( DEF_OBJECT_SIZE );
            }

            final NfsOption options = new NfsOption( );
            if( nfsSettings.getOptions() == null || nfsSettings.getOptions().isEmpty() )
            {
                options.setOptions( "rw,sync,no_acl,no_all_squash,no_root_squash" );
            }
            else
            {
                options.setOptions( nfsSettings.getOptions() );
            }

            if( nfsSettings.getClients() == null || nfsSettings.getClients().isEmpty() )
            {
                options.setClient( "*" );
            }
            else
            {
                options.setClient( nfsSettings.getClients() );
            }

            internalSettings.setNfsOptions( options );
            internalSettings.setVolumeType( VolumeType.NFS );
        }
        else // Object Volume
        {
            VolumeSettingsObject objectSettings = ( VolumeSettingsObject ) externalVolume.getSettings();

            Size maxObjSize = objectSettings.getMaxObjectSize();

            if ( maxObjSize != null &&
                 maxObjSize.getValue( SizeUnit.B ).longValue() >= Size.of( 4, SizeUnit.KB )
                                                                      .getValue( SizeUnit.B )
                                                                      .longValue() &&
                 maxObjSize.getValue( SizeUnit.B ).longValue() <= Size.of( 8, SizeUnit.MB )
                                                                      .getValue( SizeUnit.B )
                                                                      .longValue() )
            {
                internalSettings.setMaxObjectSizeInBytes( maxObjSize.getValue( SizeUnit.B )
                                                                    .intValue() );
            }
            else
            {
                logger.warn( OBJECT_SIZE_NOT_SET );
                internalSettings.setMaxObjectSizeInBytes( DEF_OBJECT_SIZE );
            }

            internalSettings.setVolumeType( VolumeType.OBJECT );
        }

        internalSettings.setMediaPolicy( convertToInternalMediaPolicy( externalVolume.getMediaPolicy() ) );
        internalSettings.setContCommitlogRetention( externalVolume.getDataProtectionPolicy().getCommitLogRetention()
                                                                  .getSeconds() );
        return internalSettings;
    }

    public static FDSP_VolumeDescType convertToInternalVolumeDescType( Volume externalVolume ) {

        FDSP_VolumeDescType volumeType = new FDSP_VolumeDescType();

        /*
         * FS-4354: allow continue commit log retention to be 0;
         *
         * but really we should allow it to be set to any valid value
         */
        volumeType.setContCommitlogRetention( externalVolume.getDataProtectionPolicy()
                                                            .getCommitLogRetention()
                                                            .getSeconds() );

        if ( externalVolume.getCreated() != null ) {
            volumeType.setCreateTime( externalVolume.getCreated().toEpochMilli() );
        }
        volumeType.setIops_assured( externalVolume.getQosPolicy().getIopsMin() );
        volumeType.setIops_throttle( externalVolume.getQosPolicy().getIopsMax() );

        FDSP_MediaPolicy fdspMediaPolicy;

        switch ( externalVolume.getMediaPolicy() ) {
            case HYBRID:
                fdspMediaPolicy = FDSP_MediaPolicy.FDSP_MEDIA_POLICY_HYBRID;
                break;
            case SSD:
                fdspMediaPolicy = FDSP_MediaPolicy.FDSP_MEDIA_POLICY_SSD;
                break;
            case HDD:
            default:
                fdspMediaPolicy = FDSP_MediaPolicy.FDSP_MEDIA_POLICY_HDD;
                break;
        }

        volumeType.setMediaPolicy( fdspMediaPolicy );

        volumeType.setLocalDomainId( 0 );
        volumeType.setRel_prio( externalVolume.getQosPolicy().getPriority() );
        volumeType.setVolUUID( externalVolume.getId() );
        volumeType.setVol_name( externalVolume.getName() );

        VolumeSettings settings = externalVolume.getSettings();
        applyVolumeSettings( settings, volumeType );

        if ( externalVolume.getStatus() != null && externalVolume.getStatus().getState() != null ) {
            volumeType.setState( convertToInternalVolumeState( externalVolume.getStatus().getState() ) );
        }

        if ( externalVolume.getTenant() != null ) {
            volumeType.setTennantId( externalVolume.getTenant().getId().intValue() );
        }

        return volumeType;
    }

    /**
     * Apply the volume settings to the internal Volume Type descriptor.
     *
     * @param settings
     * @param volumeType
     */
    protected static void applyVolumeSettings( VolumeSettings settings, FDSP_VolumeDescType volumeType ) {
        if( settings instanceof VolumeSettingsISCSI )
        {
            VolumeSettingsISCSI iscsi = ( VolumeSettingsISCSI ) settings;

            if ( iscsi.getBlockSize( ) != null )
            {
                volumeType.setMaxObjSizeInBytes( iscsi.getBlockSize( )
                                                      .getValue( SizeUnit.B )
                                                      .intValue( ) );
            }
            else
            {
                volumeType.setMaxObjSizeInBytes( DEF_BLOCK_SIZE );
            }

            volumeType.setCapacity( iscsi.getCapacity( )
                                         .getValue( SizeUnit.B )
                                         .longValue( ) );
            volumeType.setVolType( FDSP_VolType.FDSP_VOL_ISCSI_TYPE );

            final IScsiTarget target =
                new IScsiTarget(
                    convertToInternalLogicalUnitNumber( iscsi.getTarget().getLuns() ) );

            target.setInitiators( convertToInternalInitiators( iscsi.getTarget().getInitiators() ) );
            target.setIncomingUsers( convertToInternalIncomingUsers( iscsi.getTarget().getIncomingUsers() ) );
            target.setOutgoingUsers( convertToInternalOutgoingUsers( iscsi.getTarget().getOutgoingUsers() ) );

            volumeType.setIscsi( target );
            volumeType.setVolType( FDSP_VolType.FDSP_VOL_ISCSI_TYPE );

        } else if ( settings instanceof VolumeSettingsBlock ) {
            VolumeSettingsBlock blockSettings = ( VolumeSettingsBlock ) settings;

            if ( blockSettings.getBlockSize( ) != null )
            {
                volumeType.setMaxObjSizeInBytes( blockSettings.getBlockSize( )
                                                              .getValue( SizeUnit.B )
                                                              .intValue( ) );
            }
            else
            {
                volumeType.setMaxObjSizeInBytes( DEF_BLOCK_SIZE );
            }

            volumeType.setCapacity( blockSettings.getCapacity( )
                                                 .getValue( SizeUnit.B )
                                                 .longValue( ) );
            volumeType.setVolType( FDSP_VolType.FDSP_VOL_BLKDEV_TYPE );
        } else if( settings instanceof VolumeSettingsNfs ) {
            VolumeSettingsNfs nfs = ( VolumeSettingsNfs ) settings;

            final String options = nfs.getOptions();
            final String clients = nfs.getClients();

            if ( options != null && clients == null )
            {
                volumeType.setNfs( convertToInternalNfsOptions( options, "*" ) );
            }
            else if ( options != null )
            {
                volumeType.setNfs( convertToInternalNfsOptions( options, clients ) );
            }

            if ( nfs.getMaxObjectSize() != null ) {
                volumeType.setMaxObjSizeInBytes( nfs.getMaxObjectSize().getValue( SizeUnit.B ).intValue() );
            } else {
                volumeType.setMaxObjSizeInBytes( DEF_OBJECT_SIZE );
            }

            volumeType.setVolType( FDSP_VolType.FDSP_VOL_NFS_TYPE );
        } else {
            VolumeSettingsObject objectSettings = (VolumeSettingsObject) settings;

            if ( objectSettings.getMaxObjectSize() != null ) {
                volumeType.setMaxObjSizeInBytes( objectSettings.getMaxObjectSize().getValue( SizeUnit.B ).intValue() );
            } else {
                volumeType.setMaxObjSizeInBytes( DEF_OBJECT_SIZE );
            }

            volumeType.setVolType( FDSP_VolType.FDSP_VOL_S3_TYPE );
        }
    }

    /**
     * Getting the possible firebreak events for this volume in the past 24 hours
     *
     * @param v the {@link Volume}
     *
     * @return Returns EnumMap<FirebreakType, VolumeDatapointPair>
     */
    private static EnumMap<FirebreakType, VolumeDatapointPair> getFirebreakEventsMetrics( Volume v ) {

        Map<Long, EnumMap<FirebreakType, VolumeDatapointPair>> results;
        results = getFirebreakEventsMetrics( Collections.singletonList( v ) );

        return results.get( v.getId() );
    }

    /**
     * Query the firebreak events based on the metrics database.
     * <p/>
     * This queries 24 hours worth of metrics for all volumes and is potentially a huge amount of data.
     *
     * @return the firebreak events based on querying the metrics database
     */
    private static Map<Long, EnumMap<FirebreakType, VolumeDatapointPair>> getFirebreakEventsMetrics() {
        return getFirebreakEventsMetrics( Collections.emptyList() );
    }

    /**
     * Query the firebreak events based on the metrics database.
     * <p/>
     * This queries 24 hours of metrics for zero or more volumes
     * and is potentially a huge amount of data.
     *
     * @return the firebreak events based on querying the metrics database
     */
    private static Map<Long,
                          EnumMap<FirebreakType,
                                     VolumeDatapointPair>> getFirebreakEventsMetrics( List<Volume> volumes ) {

        MetricQueryCriteria query = new MetricQueryCriteria(QueryType.FIREBREAK_METRIC);
        DateRange range = DateRange.last24Hours();

        query.setSeriesType( new ArrayList<>( Metrics.FIREBREAK ) );
        query.setRange( range );

        if ( volumes != null && !volumes.isEmpty() ) {
            query.setContexts( volumes );
        }

        MetricRepository repo = SingletonRepositoryManager.instance().getMetricsRepository();

        final List<? extends IVolumeDatapoint> queryResults = repo.query( query );

        FirebreakHelper fbh = new FirebreakHelper();
        Map<Long, EnumMap<FirebreakType, VolumeDatapointPair>> map = null;

        try {
            map = fbh.findFirebreakEvents( queryResults );
        } catch ( Exception e ) {

            // TODO: at least this is logged, but I think it should be the caller that decides how to handle
            logger.error( "Error occurred while trying to retrieve firebreak events.", e );
        }

        return map;
    }

    /**
     * Get any firebreak events that have occurred over the last 24 hours by querying the event repository
     *
     * @return the map of firebreak events by volume id
     */
    private static Map<Long, EnumMap<FirebreakType, FirebreakEvent>> getFirebreakEvents() {
        return SingletonRepositoryManager.instance().getEventRepository().findLatestFirebreaks();
    }

    private static ConfigurationApi getConfigApi() {

        if ( configApi == null ) {
            configApi = SingletonConfigAPI.instance().api();
        }

        return configApi;
    }
}

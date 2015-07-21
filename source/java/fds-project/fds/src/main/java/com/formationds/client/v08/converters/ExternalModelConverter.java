package com.formationds.client.v08.converters;

import com.formationds.apis.FDSP_GetVolInfoReqType;
import com.formationds.apis.LocalDomain;
import com.formationds.apis.VolumeDescriptor;
import com.formationds.apis.VolumeType;
import com.formationds.client.ical.RecurrenceRule;
import com.formationds.client.v08.model.DataProtectionPolicy;
import com.formationds.client.v08.model.Domain;
import com.formationds.client.v08.model.Domain.DomainState;
import com.formationds.client.v08.model.MediaPolicy;
import com.formationds.client.v08.model.QosPolicy;
import com.formationds.client.v08.model.Size;
import com.formationds.client.v08.model.SizeUnit;
import com.formationds.client.v08.model.Snapshot;
import com.formationds.client.v08.model.SnapshotPolicy;
import com.formationds.client.v08.model.SnapshotPolicy.SnapshotPolicyType;
import com.formationds.client.v08.model.Tenant;
import com.formationds.client.v08.model.User;
import com.formationds.client.v08.model.Volume;
import com.formationds.client.v08.model.VolumeAccessPolicy;
import com.formationds.client.v08.model.VolumeSettings;
import com.formationds.client.v08.model.VolumeSettingsBlock;
import com.formationds.client.v08.model.VolumeSettingsObject;
import com.formationds.client.v08.model.VolumeState;
import com.formationds.client.v08.model.VolumeStatus;
import com.formationds.commons.events.FirebreakType;
import com.formationds.commons.model.DateRange;
import com.formationds.commons.model.entity.IVolumeDatapoint;
import com.formationds.commons.model.type.Metrics;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.om.repository.MetricRepository;
import com.formationds.om.repository.SingletonRepositoryManager;
import com.formationds.om.repository.helper.FirebreakHelper;
import com.formationds.om.repository.helper.FirebreakHelper.VolumeDatapointPair;
import com.formationds.om.repository.query.MetricQueryCriteria;
import com.formationds.protocol.FDSP_MediaPolicy;
import com.formationds.protocol.FDSP_VolType;
import com.formationds.protocol.FDSP_VolumeDescType;
import com.formationds.protocol.ResourceState;
import com.formationds.util.thrift.ConfigurationApi;

import org.apache.thrift.TException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.text.ParseException;
import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.EnumMap;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.TimeUnit;

@SuppressWarnings( "unused" )
public class ExternalModelConverter {

	private static ConfigurationApi configApi;
	private static final Integer DEF_BLOCK_SIZE = ( 1024 * 128 );
	private static final Integer DEF_OBJECT_SIZE = ( ( 1024 * 1024 ) * 2 );
	private static final Logger logger = LoggerFactory.getLogger( ExternalModelConverter.class );

	public static Domain convertToExternalDomain( LocalDomain internalDomain ) {

		String extName = internalDomain.getName();
		Long extId = internalDomain.getId();
		String site = internalDomain.getSite();

		Domain externalDomain = new Domain( extId, extName, site, DomainState.UP );

		return externalDomain;
	}

	public static LocalDomain convertToInternalDomain( Domain externalDomain ) {

		LocalDomain internalDomain = new LocalDomain();

		internalDomain.setId( externalDomain.getId() );
		internalDomain.setName( externalDomain.getName() );
		internalDomain.setSite( externalDomain.getName() );

		return internalDomain;
	}

	public static User convertToExternalUser( com.formationds.apis.User internalUser ) {

		Long extId = internalUser.getId();
		String extName = internalUser.getIdentifier();
		Long roleId = 1L;

		if ( internalUser.isFdsAdmin ) {
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

	public static VolumeStatus convertToExternalVolumeStatus( VolumeDescriptor internalVolume ) {

		VolumeState volumeState = convertToExternalVolumeState( internalVolume.getState() );

		final Optional<com.formationds.apis.VolumeStatus> optionalStatus =
				SingletonRepositoryManager.instance()
				.getMetricsRepository().getLatestVolumeStatus( internalVolume.getName() );

		Size extUsage = Size.of( 0L, SizeUnit.B );

		if ( optionalStatus.isPresent() ) {
			com.formationds.apis.VolumeStatus internalStatus = optionalStatus.get();

			extUsage = Size.of( internalStatus.getCurrentUsageInBytes(), SizeUnit.B );
		}

		Volume fakeVolume = new Volume( internalVolume.getVolId(), internalVolume.getName(),
				null, null, null, null, null, null, null, null, null, null );

		final EnumMap<FirebreakType, VolumeDatapointPair> fbResults = getFirebreakEvents( fakeVolume );

		Instant[] instants = {Instant.EPOCH, Instant.EPOCH};

		if ( fbResults != null ) {

			// put each firebreak event in the JSON object if it exists
			fbResults.forEach( ( type, pair ) -> {

				if ( type.equals( FirebreakType.CAPACITY ) ) {
					instants[0] = Instant.ofEpochMilli( pair.getDatapoint().getY().longValue() );
				} else if ( type.equals( FirebreakType.PERFORMANCE ) ) {
					instants[1] = Instant.ofEpochMilli( pair.getDatapoint().getY().longValue() );
				}
			} );
		}

		VolumeStatus externalStatus = new VolumeStatus( volumeState, extUsage, instants[0], instants[1] );

		return externalStatus;
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

	public static com.formationds.protocol.Snapshot convertToInternalSnapshot( Snapshot snapshot ){

		com.formationds.protocol.Snapshot internalSnapshot = new com.formationds.protocol.Snapshot();

		internalSnapshot.setCreationTimestamp( snapshot.getCreationTime().toEpochMilli() );
		internalSnapshot.setRetentionTimeSeconds( snapshot.getRetention().getSeconds() );
		internalSnapshot.setSnapshotId( snapshot.getId() );
		internalSnapshot.setSnapshotName( snapshot.getName() );
		internalSnapshot.setVolumeId( snapshot.getVolumeId() );

		return internalSnapshot;
	}

	public static Snapshot convertToExternalSnapshot( com.formationds.protocol.Snapshot protoSnapshot ){

		long creation = protoSnapshot.getCreationTimestamp();
		long retentionInSeconds = protoSnapshot.getRetentionTimeSeconds();
		long volumeId = protoSnapshot.getVolumeId();
		String snapshotName = protoSnapshot.getSnapshotName();
		long snapshotId = protoSnapshot.getSnapshotId();

		Snapshot externalSnapshot = new Snapshot( snapshotId, 
				snapshotName, 
				volumeId,
				Duration.ofSeconds( retentionInSeconds ),
				Instant.ofEpochMilli( creation ) );

		return externalSnapshot;
	}

	public static SnapshotPolicy convertToExternalSnapshotPolicy( com.formationds.apis.SnapshotPolicy internalPolicy ){

		Long extId = internalPolicy.getId();
		String intRule = internalPolicy.getRecurrenceRule();
		Long intRetention = internalPolicy.getRetentionTimeSeconds();

		RecurrenceRule extRule = new RecurrenceRule();

		try {
			extRule = extRule.parser( intRule );
		} catch (ParseException e) {
			logger.warn( "Could not parse the retention rule: " + intRule );
			logger.debug( e.getMessage(), e );
		}

		Duration extRetention = Duration.ofSeconds( intRetention );

		SnapshotPolicyType type = SnapshotPolicyType.USER;

		if ( internalPolicy.getPolicyName().contains( SnapshotPolicyType.SYSTEM_TIMELINE.name() ) ) {
			type = SnapshotPolicyType.SYSTEM_TIMELINE;
		}

		SnapshotPolicy externalPolicy = new SnapshotPolicy( extId, internalPolicy.getPolicyName(), type, extRule, extRetention );

		return externalPolicy;
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

	public static DataProtectionPolicy convertToExternalProtectionPolicy( VolumeDescriptor internalVolume ) {

		Duration extCommitRetention = Duration.ofDays( 1L );

		try {
			FDSP_VolumeDescType volDescType =
					getConfigApi().GetVolInfo( new FDSP_GetVolInfoReqType( internalVolume.getName(), 0 ) );

			long clRetention = volDescType.getContCommitlogRetention();

			extCommitRetention = Duration.ofSeconds( clRetention );
		} catch ( TException e ) {
			
			logger.warn( "Error occurred while trying to retrieve volume: " + internalVolume.getVolId() );
			logger.debug( "Exception: ", e );
			
		}

		// snapshot policies
		List<SnapshotPolicy> extPolicies = new ArrayList<>();

		try {
			final List<com.formationds.apis.SnapshotPolicy> internalPolicies =
					getConfigApi().listSnapshotPoliciesForVolume( internalVolume.getVolId() );

			for ( com.formationds.apis.SnapshotPolicy internalPolicy : internalPolicies ) {

				SnapshotPolicy externalPolicy = convertToExternalSnapshotPolicy( internalPolicy );
				extPolicies.add( externalPolicy );
			}
		} catch ( TException e ) {
			
			logger.warn( "Error occurred while trying to retrieve snapshot policies for volume: " + internalVolume.getVolId() );
			logger.debug("Exception: ", e);
		}

		DataProtectionPolicy extProtectionPolicy = new DataProtectionPolicy( extCommitRetention, extPolicies );

		return extProtectionPolicy;
	}

	public static Volume convertToExternalVolume( VolumeDescriptor internalVolume ) {

		String extName = internalVolume.getName();
		Long volumeId = internalVolume.getVolId();

		com.formationds.apis.VolumeSettings settings = internalVolume.getPolicy();
		MediaPolicy extPolicy = convertToExternalMediaPolicy( settings.getMediaPolicy() );

		VolumeSettings extSettings = null;

		if ( settings.getVolumeType().equals( VolumeType.BLOCK ) ) {

			Size capacity = Size.of( settings.getBlockDeviceSizeInBytes(), SizeUnit.B );
			Size blockSize = Size.of( settings.getMaxObjectSizeInBytes(), SizeUnit.B );

			extSettings = new VolumeSettingsBlock( capacity, blockSize );
		} else if ( settings.getVolumeType().equals( VolumeType.OBJECT ) ) {
			Size maxObjectSize = Size.of( settings.getMaxObjectSizeInBytes(), SizeUnit.B );

			extSettings = new VolumeSettingsObject( maxObjectSize );
		}

		// need to get a different volume object for the other stuff...
		// NOTE:  This won't work when we have multiple domains.
		// TODO: FIx for multiple days
		QosPolicy extQosPolicy = null;

		try {
			FDSP_VolumeDescType volDescType = getConfigApi().GetVolInfo( new FDSP_GetVolInfoReqType( extName, 0 ) );

			extQosPolicy = new QosPolicy( volDescType.getRel_prio(),
					(int) volDescType.getIops_assured(),
					(int) volDescType.getIops_throttle() );
		} catch ( Exception e ) {
			
			logger.warn( "Error occurred while trying to get VolInfo type for volume: " + internalVolume.getVolId() );
			logger.debug( "Exception: ", e );
		}

		Tenant extTenant = null;

		try {
			List<com.formationds.apis.Tenant> internalTenants = getConfigApi().listTenants( 0 );

			for ( com.formationds.apis.Tenant internalTenant : internalTenants ) {

				if ( internalTenant.getId() == internalVolume.getTenantId() ) {
					extTenant = convertToExternalTenant( internalTenant );
					break;
				}
			}
		} catch ( TException e ) {
			
			logger.warn( "Error occurred while trying to retrieve a list of tenants." );
			logger.debug( "Exception: ", e );
		}

		VolumeStatus extStatus = convertToExternalVolumeStatus( internalVolume );

		VolumeAccessPolicy extAccessPolicy = VolumeAccessPolicy.exclusiveRWPolicy();

		DataProtectionPolicy extProtectionPolicy = convertToExternalProtectionPolicy( internalVolume );

		Instant extCreation = Instant.ofEpochMilli( internalVolume.dateCreated );

		Volume externalVolume = new Volume( volumeId,
				extName,
				extTenant,
				"Application",
				extStatus,
				extSettings,
				extPolicy,
				extProtectionPolicy,
				extAccessPolicy,
				extQosPolicy,
				extCreation,
				null );

		return externalVolume;
	}

	public static VolumeDescriptor convertToInternalVolumeDescriptor( Volume externalVolume ) {

		VolumeDescriptor internalDescriptor = new VolumeDescriptor();

		if ( externalVolume.getCreated() != null ){
			internalDescriptor.setDateCreated( externalVolume.getCreated().toEpochMilli() );
		}

		internalDescriptor.setName( externalVolume.getName() );

		if ( externalVolume.getStatus() != null && externalVolume.getStatus().getState() != null ){
			internalDescriptor.setState( convertToInternalVolumeState( externalVolume.getStatus().getState() ) );
		}

		if ( externalVolume.getTenant() != null ) {
			internalDescriptor.setTenantId( externalVolume.getTenant().getId() );
		}

		if ( externalVolume.getId() != null ){
			internalDescriptor.setVolId( externalVolume.getId() );
		}

		com.formationds.apis.VolumeSettings internalSettings = new com.formationds.apis.VolumeSettings();

		// block volume
		if ( externalVolume.getSettings() instanceof VolumeSettingsBlock ) {

			VolumeSettingsBlock blockSettings = (VolumeSettingsBlock) externalVolume.getSettings();

			internalSettings.setBlockDeviceSizeInBytes( blockSettings.getCapacity().getValue( SizeUnit.B ).longValue() );

			Size blockSize = blockSettings.getBlockSize();
			
			if ( blockSize != null &&
					blockSize.getValue( SizeUnit.B ).longValue() > Size.of( 4, SizeUnit.KB ).getValue( SizeUnit.B ).longValue() &&
					blockSize.getValue( SizeUnit.B ).longValue() < Size.of( 8, SizeUnit.MB ).getValue( SizeUnit.B ).longValue()){
				internalSettings.setMaxObjectSizeInBytes( blockSize.getValue( SizeUnit.B )
						.intValue() );
			}
			else {
				logger.warn( "Block size was either not set, or outside the bounds of 4KB-8MB so it is being set to the default of 128KB");
				internalSettings.setMaxObjectSizeInBytes( DEF_BLOCK_SIZE );
			}

			internalSettings.setVolumeType( VolumeType.BLOCK );

			// object volume
		} else {
			VolumeSettingsObject objectSettings = (VolumeSettingsObject) externalVolume.getSettings();

			Size maxObjSize = objectSettings.getMaxObjectSize();
			
			if ( maxObjSize != null && 
					maxObjSize.getValue( SizeUnit.B ).longValue() > Size.of( 4, SizeUnit.KB ).getValue( SizeUnit.B ).longValue() &&
					maxObjSize.getValue( SizeUnit.B ).longValue() < Size.of( 8, SizeUnit.MB ).getValue( SizeUnit.B ).longValue() ){
				internalSettings.setMaxObjectSizeInBytes( maxObjSize.getValue( SizeUnit.B ).intValue() );
			}
			else {
				logger.warn( "Maximum object size was either not set, or outside the bounds of 4KB-8MB so it is being set to the default of 2MB");
				internalSettings.setMaxObjectSizeInBytes( DEF_OBJECT_SIZE );
			}

			internalSettings.setVolumeType( VolumeType.OBJECT );
		}

		internalSettings.setMediaPolicy( convertToInternalMediaPolicy( externalVolume.getMediaPolicy() ) );
		internalSettings.setContCommitlogRetention( externalVolume.getDataProtectionPolicy().getCommitLogRetention()
				.getSeconds() );

		internalDescriptor.setPolicy( internalSettings );

		return internalDescriptor;
	}

	public static FDSP_VolumeDescType convertToInternalVolumeDescType( Volume externalVolume ) {

		FDSP_VolumeDescType volumeType = new FDSP_VolumeDescType();

		volumeType.setContCommitlogRetention( externalVolume.getDataProtectionPolicy().getCommitLogRetention()
				.getSeconds() );

		if ( externalVolume.getCreated() != null ){
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

		if ( settings instanceof VolumeSettingsBlock ) {
			VolumeSettingsBlock blockSettings = (VolumeSettingsBlock) settings;

			if ( blockSettings.getBlockSize() != null ){
				volumeType.setMaxObjSizeInBytes( blockSettings.getBlockSize().getValue( SizeUnit.B ).intValue() );
			}
			else {
				volumeType.setMaxObjSizeInBytes( DEF_BLOCK_SIZE );
			}

			volumeType.setCapacity( blockSettings.getCapacity().getValue( SizeUnit.B ).longValue() );
			volumeType.setVolType( FDSP_VolType.FDSP_VOL_BLKDEV_TYPE );
		} else {
			VolumeSettingsObject objectSettings = (VolumeSettingsObject) settings;

			if ( objectSettings.getMaxObjectSize() != null ){
				volumeType.setMaxObjSizeInBytes( objectSettings.getMaxObjectSize().getValue( SizeUnit.B ).intValue() );
			}
			else {
				volumeType.setMaxObjSizeInBytes( DEF_OBJECT_SIZE );
			}

			volumeType.setVolType( FDSP_VolType.FDSP_VOL_S3_TYPE );
		}

		if ( externalVolume.getStatus() != null && externalVolume.getStatus().getState() != null ){
			volumeType.setState( convertToInternalVolumeState( externalVolume.getStatus().getState() ) );
		}

		if ( externalVolume.getTenant() != null ){
			volumeType.setTennantId( externalVolume.getTenant().getId().intValue() );
		}

		return volumeType;
	}

	/**
	 * Getting the possible firebreak events for this volume in the past 24 hours
	 *
	 * @param v the {@link Volume}
	 *
	 * @return Returns EnumMap<FirebreakType, VolumeDatapointPair>
	 */
	private static EnumMap<FirebreakType, VolumeDatapointPair> getFirebreakEvents( Volume v ) {

		MetricQueryCriteria query = new MetricQueryCriteria();
		DateRange range = new DateRange();
		range.setEnd( TimeUnit.MILLISECONDS.toSeconds( (new Date().getTime()) ) );
		range.setStart( range.getEnd() - TimeUnit.DAYS.toSeconds( 1 ) );

		query.setSeriesType( new ArrayList<>( Metrics.FIREBREAK ) );
		query.setContexts( Collections.singletonList( v ) );
		query.setRange( range );

		MetricRepository repo = SingletonRepositoryManager.instance().getMetricsRepository();

		final List<? extends IVolumeDatapoint> queryResults = repo.query( query );

		FirebreakHelper fbh = new FirebreakHelper();
		EnumMap<FirebreakType, VolumeDatapointPair> map = null;

		try {
			map = fbh.findFirebreakEvents( queryResults ).get( v.getId() );
		} catch ( TException e ) {
			
			logger.error( "Error occurred while trying to retrieve firebreak events.", e );
		}

		return map;
	}

	private static ConfigurationApi getConfigApi() {

		if ( configApi == null ) {
			configApi = SingletonConfigAPI.instance().api();
		}

		return configApi;
	}
}

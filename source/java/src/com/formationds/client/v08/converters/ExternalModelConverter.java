package com.formationds.client.v08.converters;

import java.text.ParseException;
import java.time.Instant;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.EnumMap;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.TimeUnit;
import java.time.Duration;

import org.apache.thrift.TException;

import com.formationds.apis.FDSP_GetVolInfoReqType;
import com.formationds.apis.FDSP_VolumeNotFound;
import com.formationds.apis.LocalDomain;
import com.formationds.apis.VolumeDescriptor;
import com.formationds.apis.VolumeType;
import com.formationds.client.ical.RecurrenceRule;
import com.formationds.client.v08.model.DataProtectionPolicy;
import com.formationds.client.v08.model.Domain;
import com.formationds.client.v08.model.MediaPolicy;
import com.formationds.client.v08.model.Node;
import com.formationds.client.v08.model.QosPolicy;
import com.formationds.client.v08.model.Role;
import com.formationds.client.v08.model.Service;
import com.formationds.client.v08.model.ServiceType;
import com.formationds.client.v08.model.Size;
import com.formationds.client.v08.model.SizeUnit;
import com.formationds.client.v08.model.SnapshotPolicy;
import com.formationds.client.v08.model.Tenant;
import com.formationds.client.v08.model.User;
import com.formationds.client.v08.model.Volume;
import com.formationds.client.v08.model.VolumeAccessPolicy;
import com.formationds.client.v08.model.VolumeSettings;
import com.formationds.client.v08.model.VolumeState;
import com.formationds.client.v08.model.VolumeStatus;
import com.formationds.client.v08.model.Service.ServiceStatus;
import com.formationds.client.v08.model.VolumeSettings.BlockVolumeSettings;
import com.formationds.client.v08.model.VolumeSettings.ObjectVolumeSettings;
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
import com.formationds.protocol.FDSP_MgrIdType;
import com.formationds.protocol.FDSP_NodeState;
import com.formationds.protocol.FDSP_Node_Info_Type;
import com.formationds.protocol.FDSP_VolumeDescType;
import com.formationds.protocol.ResourceState;
import com.formationds.util.thrift.ConfigurationApi;

public class ExternalModelConverter {

	private static ConfigurationApi configApi;
	
	public static Domain convertToExternalDomain( LocalDomain internalDomain ){
		
		String extName = internalDomain.getName();
		Long extId = internalDomain.getId();
		String site = internalDomain.getSite();
		
		Domain externalDomain = new Domain( extId, extName, site );
		
		return externalDomain;
	}
	
	public static LocalDomain convertToInternalDomain( Domain externalDomain ){
		
		LocalDomain internalDomain = new LocalDomain();
		
		internalDomain.setId( externalDomain.getId() );
		internalDomain.setName( externalDomain.getName() );
		internalDomain.setSite( externalDomain.getName() );
		
		return internalDomain;
	}
	
	public static User convertToExternalUser( com.formationds.apis.User internalUser ){
		
		Long extId = internalUser.getId();
		String extName = internalUser.getIdentifier();
		Role roleDescriptor = Role.USER;
		
		if ( internalUser.isFdsAdmin ){
			roleDescriptor = Role.ADMIN;
		}
		
		Optional<com.formationds.apis.Tenant> tenant = getConfigApi().tenantFor( extId );
		
		Tenant externalTenant = null;
		
		if ( tenant.isPresent() ){
			
			externalTenant = convertToExternalTenant( tenant.get() );
		}

		User externalUser = new User( extId, extName, roleDescriptor, externalTenant );
		
		return externalUser;
	}
	
	public static com.formationds.apis.User convertToInternalUser( User externalUser ){
		
		com.formationds.apis.User internalUser = new com.formationds.apis.User();
		
		internalUser.setId( externalUser.getId() );
		internalUser.setIdentifier( externalUser.getName() );

		if ( externalUser.getRoleDescriptor().name().equals( Role.ADMIN ) ){
			internalUser.setIsFdsAdmin( true );
		}
		else {
			internalUser.setIsFdsAdmin( false );
		}
		
		return internalUser;
	}
	
	public static Tenant convertToExternalTenant( com.formationds.apis.Tenant internalTenant ){
		
		Long id = internalTenant.getId();
		String name = internalTenant.getIdentifier();
		
		Tenant externalTenant = new Tenant( id, name );
		return externalTenant;
	}
	
	public static com.formationds.apis.Tenant convertToInternalTenant( Tenant externalTenant ){
		
		com.formationds.apis.Tenant internalTenant = new com.formationds.apis.Tenant();
		internalTenant.setId( externalTenant.getId() );
		internalTenant.setIdentifier( externalTenant.getName() );
		
		return internalTenant;
	}
	
	public static Optional<ServiceType> convertToExternalServiceType( FDSP_MgrIdType type ){
		
		Optional<ServiceType> externalType;
		
		switch( type ){
			case FDSP_ACCESS_MGR:
				externalType = Optional.of( ServiceType.AM );
				break;
			case FDSP_DATA_MGR:
				externalType = Optional.of( ServiceType.DM );
				break;
			case FDSP_ORCH_MGR:
				externalType = Optional.of( ServiceType.OM );
				break;
			case FDSP_PLATFORM:
				externalType = Optional.of( ServiceType.PM );
				break;
			case FDSP_STOR_MGR:
				externalType = Optional.of( ServiceType.SM );
				break;
			default:
				externalType = Optional.empty();
		}
		
		return externalType;
	}
	
	public static Optional<FDSP_MgrIdType> convertToInternalServiceType( ServiceType externalType ){
		
		Optional<FDSP_MgrIdType> internalType;
		
		switch( externalType ){
			case AM:
				internalType = Optional.of( FDSP_MgrIdType.FDSP_ACCESS_MGR );
				break;
			case DM:
				internalType = Optional.of( FDSP_MgrIdType.FDSP_DATA_MGR );
				break;
			case OM:
				internalType = Optional.of( FDSP_MgrIdType.FDSP_ORCH_MGR );
				break;
			case PM: 
				internalType = Optional.of( FDSP_MgrIdType.FDSP_PLATFORM );
				break;
			case SM:
				internalType = Optional.of( FDSP_MgrIdType.FDSP_STOR_MGR );
				break;
			default:
				internalType = Optional.empty();
		}
		
		return internalType;
	}
	
	public static Optional<ServiceStatus> convertToExternalServiceStatus( FDSP_NodeState internalState ){
		
		Optional<ServiceStatus> externalStatus;
		
		switch( internalState ){
			case FDS_Node_Down:
				externalStatus = Optional.of( ServiceStatus.NOT_RUNNING );
				break;
			case FDS_Node_Up:
				externalStatus = Optional.of( ServiceStatus.RUNNING );
				break;
			case FDS_Node_Rmvd:
				externalStatus = Optional.of( ServiceStatus.UNREACHABLE );
				break;
			case FDS_Start_Migration:
				externalStatus = Optional.of( ServiceStatus.INITIALIZING );
				break;
			case FDS_Node_Discovered:
				externalStatus = Optional.of( ServiceStatus.NOT_RUNNING );
			default:
				externalStatus = Optional.of( ServiceStatus.RUNNING );
		}
		
		return externalStatus;
	}
	
	public static Optional<FDSP_NodeState> convertToInternalServiceStatus( ServiceStatus externalStatus ){
		
		Optional<FDSP_NodeState> internalState;
		
		switch( externalStatus ){
			case DEGRADED:
			case LIMITED:
			case NOT_RUNNING:
			case ERROR:
			case UNEXPECTED_EXIT:
				internalState = Optional.of( FDSP_NodeState.FDS_Node_Down );
				break;
			case UNREACHABLE:
				internalState = Optional.of( FDSP_NodeState.FDS_Node_Rmvd );
				break;
			case INITIALIZING:
				internalState = Optional.of( FDSP_NodeState.FDS_Start_Migration );
				break;
			default:
				internalState = Optional.of( FDSP_NodeState.FDS_Node_Up );
				break;
		}
		
		return internalState;
	}
	
	public static Service convertToExternalService( FDSP_Node_Info_Type nodeInfoType ){
		
		Long extId = nodeInfoType.getService_uuid();
		int extControlPort = nodeInfoType.getControl_port();
		Optional<ServiceType> optType = convertToExternalServiceType( nodeInfoType.getNode_type() );
		Optional<ServiceStatus> optStatus = convertToExternalServiceStatus( nodeInfoType.getNode_state() );
		
		ServiceType extType = null;
		ServiceStatus extStatus = null;
		
		if ( optType.isPresent() ){
			extType = optType.get();
		}
		
		if ( optStatus.isPresent() ){
			extStatus = optStatus.get();
		}
		
		Service externalService = new Service( extId, extType, extControlPort, extStatus );
		
		return externalService;
	}
	
	public static FDSP_Node_Info_Type convertToInternalService( Node externalNode, Service externalService ){
		
		FDSP_Node_Info_Type nodeInfo = new FDSP_Node_Info_Type();
		
		nodeInfo.setControl_port( externalService.getPort() );
		nodeInfo.setNode_uuid( externalService.getId() );
		nodeInfo.setNode_id( externalNode.getId().intValue() );
		nodeInfo.setNode_name( externalNode.getName() );
		
		Optional<FDSP_NodeState> optState = convertToInternalServiceStatus( externalService.getStatus() );
		
		if ( optState.isPresent() ){
			nodeInfo.setNode_state( optState.get() );
		}
		
		Optional<FDSP_MgrIdType> optType = convertToInternalServiceType( externalService.getType() );
		
		if ( optType.isPresent() ){
			nodeInfo.setNode_type( optType.get() );
		}
	
		//TODO:  The IP addresses
		
		return nodeInfo;
	}
	
	public static MediaPolicy convertToExternalMediaPolicy( com.formationds.apis.MediaPolicy internalPolicy ){
		
		MediaPolicy externalPolicy;
		
		switch( internalPolicy ){
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
	
	public static com.formationds.apis.MediaPolicy convertToInternalMediaPolicy( MediaPolicy externalPolicy ){
		
		com.formationds.apis.MediaPolicy internalPolicy;
		
		switch( externalPolicy ){
		
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
	
	public static VolumeState convertToExternalVolumeState( ResourceState internalState ){
		
		VolumeState state = VolumeState.valueOf( internalState.name() );
		
		return state;
	}
	
	public static ResourceState convertToInternalVolumeState( VolumeState externalState ){
		
		return ResourceState.valueOf( externalState.name() );
	}
	
	public static VolumeStatus convertToExternalVolumeStatus( VolumeDescriptor internalVolume ){
		
		VolumeState volumeState = convertToExternalVolumeState( internalVolume.getState() );
		
		final Optional<com.formationds.apis.VolumeStatus> optionalStatus =
				SingletonRepositoryManager.instance()
					.getMetricsRepository().getLatestVolumeStatus( internalVolume.getName() );
		
		Size<Long> extUsage = new Size<>( 0L, SizeUnit.BYTE );
		
		if ( optionalStatus.isPresent() ){
			com.formationds.apis.VolumeStatus internalStatus = optionalStatus.get();
			
			extUsage = new Size<>( internalStatus.getCurrentUsageInBytes(), SizeUnit.BYTE );
		}
		
		Volume fakeVolume = new Volume( internalVolume.getVolId(), internalVolume.getName(),
				null, null, null, null, null, null, null, null, null, null);
		
		final EnumMap<FirebreakType, VolumeDatapointPair> fbResults = getFirebreakEvents( fakeVolume );
		
		Instant[] instants = {Instant.EPOCH, Instant.EPOCH};
		
		if ( fbResults != null ) {

            // put each firebreak event in the JSON object if it exists
            fbResults.forEach( (type, pair) -> {
            	
            	if ( type.equals( FirebreakType.CAPACITY ) ){
            		instants[0] = Instant.ofEpochMilli( pair.getDatapoint().getY().longValue() );
            	}
            	else if ( type.equals( FirebreakType.PERFORMANCE ) ){
            		instants[1] = Instant.ofEpochMilli( pair.getDatapoint().getY().longValue() );
            	}
            });	
        }
		
		VolumeStatus externalStatus = new VolumeStatus( volumeState, extUsage, instants[0], instants[1] );
		
		return externalStatus;
	}
	
	public static SnapshotPolicy convertToExternalSnapshotPolicy( com.formationds.apis.SnapshotPolicy internalPolicy ){
		
		Long extId = internalPolicy.getId();
		String intRule = internalPolicy.getRecurrenceRule();
		Long intRetention = internalPolicy.getRetentionTimeSeconds();
		
		RecurrenceRule extRule = new RecurrenceRule();
		
		try {
			extRule = extRule.parser( intRule );
		} catch (ParseException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
		Duration extRetention = Duration.ofSeconds( intRetention );
		
		SnapshotPolicy externalPolicy = new SnapshotPolicy( extId, extRule, extRetention );
		
		return externalPolicy;
	}
	
	public static com.formationds.apis.SnapshotPolicy convertToInternalSnapshotPolicy( SnapshotPolicy externalPolicy ){
		com.formationds.apis.SnapshotPolicy internalPolicy = new com.formationds.apis.SnapshotPolicy();
		
		internalPolicy.setId( externalPolicy.getId() );
		internalPolicy.setPolicyName( externalPolicy.getId().toString() );
		internalPolicy.setRecurrenceRule( externalPolicy.getRecurrenceRule().toString() );
		internalPolicy.setRetentionTimeSeconds( externalPolicy.getRetentionTime().getSeconds() );
		
		return internalPolicy;
	}
	
	public static DataProtectionPolicy convertToExternalProtectionPolicy( VolumeDescriptor internalVolume ){
		
		Duration extCommitRetention = Duration.ofDays( 1L );
		
		try {
			FDSP_VolumeDescType volDescType = getConfigApi().GetVolInfo( new FDSP_GetVolInfoReqType( internalVolume.getName(), 0 ) );
			
			long clRetention = volDescType.getContCommitlogRetention();
			
			extCommitRetention = Duration.ofSeconds( clRetention );
			
		} catch (TException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
		// snapshot policies
		List<SnapshotPolicy> extPolicies = new ArrayList<>();
		
        try {
			final List<com.formationds.apis.SnapshotPolicy> internalPolicies =
			        getConfigApi().listSnapshotPoliciesForVolume( internalVolume.getVolId() );
			
			for ( com.formationds.apis.SnapshotPolicy internalPolicy : internalPolicies ){
				
				SnapshotPolicy externalPolicy = convertToExternalSnapshotPolicy( internalPolicy );
				extPolicies.add( externalPolicy );
			}
			
		} catch (TException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
        DataProtectionPolicy extProtectionPolicy = new DataProtectionPolicy( extCommitRetention, extPolicies );
        
		return extProtectionPolicy;
	}
	
	public static Volume convertToExternalVolume( VolumeDescriptor internalVolume ){
		
		String extName = internalVolume.getName();
		Long volumeId = internalVolume.getVolId();
		
		com.formationds.apis.VolumeSettings settings = internalVolume.getPolicy();
		MediaPolicy extPolicy = convertToExternalMediaPolicy( settings.getMediaPolicy() );
		
		VolumeSettings extSettings = null;
		
		if ( settings.getVolumeType().equals( VolumeType.BLOCK ) ){
			
			Size<Long> capacity = new Size<>( settings.getBlockDeviceSizeInBytes(), SizeUnit.BYTE );
			Size<Integer> blockSize = new Size<>( settings.getMaxObjectSizeInBytes(), SizeUnit.BYTE );
			
			extSettings = new BlockVolumeSettings( capacity, blockSize );
		}
		else if ( settings.getVolumeType().equals( VolumeType.OBJECT ) ){
			Size<Integer> maxObjectSize = new Size<>( settings.getMaxObjectSizeInBytes(), SizeUnit.BYTE );
			
			extSettings = new ObjectVolumeSettings( maxObjectSize );
		}
		
		// need to get a different volume object for the other stuff...
		// NOTE:  This won't work when we have multiple domains.
		// TODO: FIx for multiple days
		QosPolicy extQosPolicy = null;
		
		try {
			FDSP_VolumeDescType volDescType = getConfigApi().GetVolInfo( new FDSP_GetVolInfoReqType( extName, 0 ) );
			
			extQosPolicy = new QosPolicy( volDescType.getRel_prio(), 
										  (int)volDescType.getIops_assured(), 
										  (int)volDescType.getIops_throttle() );
			
		} catch (FDSP_VolumeNotFound e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (TException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
		Tenant extTenant = null;
		
		try {
			List<com.formationds.apis.Tenant> internalTenants = getConfigApi().listTenants( 0 );
			
			for ( com.formationds.apis.Tenant internalTenant : internalTenants ){
				
				if ( internalTenant.getId() == internalVolume.getTenantId() ){
					extTenant = convertToExternalTenant( internalTenant );
					break;
				}
			}
			
		} catch (TException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
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
	
	public static VolumeDescriptor convertToInternalVolumeDescriptor( Volume externalVolume ){
		
		VolumeDescriptor internalDescriptor = new VolumeDescriptor();
		
		internalDescriptor.setDateCreated( externalVolume.getCreated().toEpochMilli() );
		internalDescriptor.setName( externalVolume.getName() );
		internalDescriptor.setState( convertToInternalVolumeState( externalVolume.getStatus().getState() ) );
		
		if ( externalVolume.getTenant() != null ){
			internalDescriptor.setTenantId( externalVolume.getTenant().getId() );
		}
		
		internalDescriptor.setVolId( externalVolume.getId() );
		
		com.formationds.apis.VolumeSettings internalSettings = new com.formationds.apis.VolumeSettings();
		
		if ( externalVolume.getSettings() instanceof BlockVolumeSettings ){
			
			BlockVolumeSettings blockSettings = (BlockVolumeSettings)externalVolume.getSettings();
			internalSettings.setBlockDeviceSizeInBytes( blockSettings.getCapacity().getValue( SizeUnit.BYTE ).longValue() );
			internalSettings.setMaxObjectSizeInBytes( blockSettings.getBlockSize().getValue( SizeUnit.BYTE ).intValue() );
		}
		else {
			ObjectVolumeSettings objectSettings = (ObjectVolumeSettings)externalVolume.getSettings();
			internalSettings.setMaxObjectSizeInBytes( objectSettings.getMaxObjectSize().getValue( SizeUnit.BYTE ).intValue() );
		}
		
		internalSettings.setMediaPolicy( convertToInternalMediaPolicy( externalVolume.getMediaPolicy() ) );
		internalSettings.setContCommitlogRetention( externalVolume.getDataProtectionPolicy().getCommitLogRetention().getSeconds() );
		
		internalDescriptor.setPolicy( internalSettings );
		
		return internalDescriptor;
	}
	
	/**
	 * Getting the possible firebreak events for this volume in the past 24 hours
	 * 
	 * @param v the {@link Volume}
	 * @return Returns EnumMap<FirebreakType, VolumeDatapointPair>
	 */
	private static EnumMap<FirebreakType, VolumeDatapointPair> getFirebreakEvents( Volume v ){

		MetricQueryCriteria query = new MetricQueryCriteria();      
		DateRange range = new DateRange();
		range.setEnd( TimeUnit.MILLISECONDS.toSeconds( (new Date().getTime() ) ) );
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
		} catch (TException e) {
			e.printStackTrace();
		}

		return map;
	}
	
	private static ConfigurationApi getConfigApi(){
		
		if ( configApi == null ){
			configApi = SingletonConfigAPI.instance().api();
		}
		
		return configApi;
	}
}

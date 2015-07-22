/*
 * Copyright (c) 2015 Formation Data Systems. All rights Reserved.
 */

package com.formationds.om.webkit.rest.v08.volumes;

import com.formationds.apis.FDSP_ModifyVolType;
import com.formationds.apis.VolumeDescriptor;
import com.formationds.client.v08.converters.ExternalModelConverter;
import com.formationds.client.v08.model.SnapshotPolicy;
import com.formationds.client.v08.model.Volume;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.protocol.ApiException;
import com.formationds.protocol.ErrorCode;
import com.formationds.protocol.FDSP_VolumeDescType;
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

import java.io.InputStreamReader;
import java.io.Reader;
import java.util.List;
import java.util.Map;

public class CreateVolume implements RequestHandler {

	private static final Logger logger = LoggerFactory
			.getLogger(CreateVolume.class);

	private final Authorizer authorizer;
	private ConfigurationApi configApi;
	private final AuthenticationToken token;

	public CreateVolume(
			final Authorizer authorizer,
			final AuthenticationToken token) {
		
		this.authorizer = authorizer;
		this.token = token;
	}

	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {

		Volume newVolume;
		
		logger.debug( "Creating a new volume." );
		
		try {
		
			final Reader bodyReader = new InputStreamReader( request.getInputStream() );
			
			newVolume = ObjectModelHelper.toObject( bodyReader, Volume.class );
			            
			logger.trace( ObjectModelHelper.toJSON( newVolume ) );
		}
		catch( Exception e ){
			logger.error( "Unable to convet the body to a valid Volume object.", e );
			throw new ApiException( "Invalid input parameters", ErrorCode.BAD_REQUEST );
		}

        validateQOSSettings( newVolume );
        
		VolumeDescriptor internalVolume = ExternalModelConverter.convertToInternalVolumeDescriptor( newVolume );
		
		final String domainName = "";
		
		// creating the volume
		try {
			
			getConfigApi().createVolume( domainName, 
										 internalVolume.getName(), 
										 internalVolume.getPolicy(), 
										 getAuthorizer().tenantId( getToken() ));
	    } catch( ApiException e ) {
	
	        if ( e.getErrorCode().equals(ErrorCode.RESOURCE_ALREADY_EXISTS)) {

	        	throw new ApiException( "A volume with this name already exists.", ErrorCode.RESOURCE_ALREADY_EXISTS );
	        }
	
	        logger.error( "CREATE::FAILED::" + e.getMessage(), e );
	
	        // allow dispatcher to handle
	        throw e;
	    } catch ( TException | SecurityException se ) {
	        logger.error( "CREATE::FAILED::" + se.getMessage(), se );
	
	        // allow dispatcher to handle
	        throw se;
	    }
		
		long volumeId = getConfigApi().getVolumeId( newVolume.getName() );
		newVolume.setId( volumeId );
		
		// setting the QOS for the volume
		try {
			setQosForVolume( newVolume );
		}
        catch( TException thriftException ){
			logger.error( "CREATE::FAILED::" + thriftException.getMessage(), thriftException );
			throw thriftException;
		}
		
		// new that we've finished all that - create and attach the snapshot policies to this volume
		try {
			createSnapshotPolicies( newVolume );
		}
		catch( TException thriftException ){
			logger.error( "CREATE::FAILED::" + thriftException.getMessage(), thriftException );
			throw thriftException;
		}
		
		List<Volume> volumes = (new ListVolumes( getAuthorizer(), getToken() )).listVolumes();
		Volume myVolume = null;
		
		for ( Volume volume : volumes ){
			if ( volume.getId().equals( volumeId ) ){
				myVolume = volume;
				break;
			}
		}
		
		String volumeString = ObjectModelHelper.toJSON( myVolume );
		
		return new TextResource( volumeString );
	}
	
	/**
	 * Handle setting the QOS for the volume in question
	 * @param externalVolume
	 * @throws ApiException
	 * @throws TException
	 */
	public void setQosForVolume( Volume externalVolume ) throws ApiException, TException{
		
	    if( externalVolume.getId() != null && externalVolume.getId() > 0 ) {

	    	try {
				Thread.sleep( 200 );
			} catch (InterruptedException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
          
	    	FDSP_VolumeDescType volumeDescType = ExternalModelConverter.convertToInternalVolumeDescType( externalVolume );
	          
	    	getConfigApi().ModifyVol( new FDSP_ModifyVolType( externalVolume.getName(), externalVolume.getId(), volumeDescType ) );    
	    }
	    else {
	    	throw new ApiException( "Could not verify volume to set QOS parameters.", ErrorCode.SERVICE_NOT_READY );
	    }
	}
	
	/**
	 * Create and attach all the snapshot policies to the newly created volume
	 * @param externalVolume
	 * @throws Exception 
	 */
	public void createSnapshotPolicies( Volume externalVolume ) throws Exception{
		
		CreateSnapshotPolicy createEndpoint = new CreateSnapshotPolicy( getAuthorizer(), getToken() );
		
		for ( SnapshotPolicy policy : externalVolume.getDataProtectionPolicy().getSnapshotPolicies() ){
			
			createEndpoint.createSnapshotPolicy( externalVolume.getId(), policy );			
		}
	}

    /**
     * @param volume the {@link Volume} representing the external model object
     *
     * @throws ApiException if the QOS settings are not valid
     */
    public void validateQOSSettings( final Volume volume )
        throws ApiException
    {
        logger.trace( "Validate QOS -- MIN: {} MAX: {}",
                      volume.getQosPolicy( )
                            .getIopsMin( ),
                      volume.getQosPolicy( )
                            .getIopsMax( ) );

        if( !( ( volume.getQosPolicy( )
                       .getIopsMax( ) == 0 ) ||
               ( volume.getQosPolicy( )
                       .getIopsMin( ) <=
                 volume.getQosPolicy( )
                       .getIopsMax( ) ) ) )
        {
            final String message = "QOS value out-of-range ( assured <= throttled )";
            logger.error( message );
            throw new ApiException( message, ErrorCode.BAD_REQUEST );
        }
    }
    
	private Authorizer getAuthorizer(){
		return this.authorizer;
	}
	
	private AuthenticationToken getToken(){
		return this.token;
	}
	
	private ConfigurationApi getConfigApi(){
		
		if ( configApi == null ){
			configApi = SingletonConfigAPI.instance().api();
		}
		
		return configApi;
	}
}

package com.formationds.om.helper;

import com.formationds.apis.MediaPolicy;
import com.formationds.protocol.svc.types.FDSP_MediaPolicy;

/**
 * This class will switch between an FDSP thrift media policy and a com.formation.apis.MediaPolicy
 * 
 * Eventually there should be no need for this class once we finalize a conversion to only use the
 * com.formation.apis thrift contracts
 * @author nate
 *
 */
public class MediaPolicyConverter {

	public static MediaPolicy convertToMediaPolicy( final FDSP_MediaPolicy fdspPolicy ){

		if ( fdspPolicy == null ) {
			return MediaPolicy.HDD_ONLY;
		}

		MediaPolicy mediaPolicy;
		
		switch( fdspPolicy ){
			case FDSP_MEDIA_POLICY_HDD:
				mediaPolicy = MediaPolicy.HDD_ONLY;
				break;
			case FDSP_MEDIA_POLICY_SSD:
				mediaPolicy = MediaPolicy.SSD_ONLY;
				break;
			case FDSP_MEDIA_POLICY_HYBRID:
				mediaPolicy = MediaPolicy.HYBRID_ONLY;
				break;
			default:
				mediaPolicy = MediaPolicy.HDD_ONLY;
				break;
		}
		
		return mediaPolicy;
	}
	
	public static FDSP_MediaPolicy convertToFDSPMediaPolicy( final MediaPolicy mediaPolicy ){

		if ( mediaPolicy == null ) {
			return FDSP_MediaPolicy.FDSP_MEDIA_POLICY_UNSET;
		}

		FDSP_MediaPolicy fdspPolicy;
		
		switch( mediaPolicy ){
			case HDD_ONLY:
				fdspPolicy = FDSP_MediaPolicy.FDSP_MEDIA_POLICY_HDD;
				break;
			case SSD_ONLY:
				fdspPolicy = FDSP_MediaPolicy.FDSP_MEDIA_POLICY_SSD;
				break;
			case HYBRID_ONLY:
				fdspPolicy = FDSP_MediaPolicy.FDSP_MEDIA_POLICY_HYBRID;
				break;
			default: 
				fdspPolicy = FDSP_MediaPolicy.FDSP_MEDIA_POLICY_UNSET;
				break;
		}
		
		return fdspPolicy;
	}
	
}

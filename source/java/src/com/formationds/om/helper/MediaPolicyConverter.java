package com.formationds.om.helper;

import FDS_ProtocolInterface.FDSP_MediaPolicy;

import com.formationds.apis.MediaPolicy;

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
		
		MediaPolicy mediaPolicy;
		
		switch( fdspPolicy ){
			case FDSP_MEDIA_POLICY_HDD:
				mediaPolicy = MediaPolicy.HDD_ONLY;
				break;
			case FDSP_MEDIA_POLICY_SSD:
				mediaPolicy = MediaPolicy.SSD_ONLY;
				break;
			default:
				mediaPolicy = MediaPolicy.HDD_ONLY;
				break;
		}
		
		return mediaPolicy;
	}
	
	public static FDSP_MediaPolicy convertToFDSPMediaPolicy( final MediaPolicy mediaPolicy ){
		
		FDSP_MediaPolicy fdspPolicy;
		
		switch( mediaPolicy ){
			case HDD_ONLY:
				fdspPolicy = FDSP_MediaPolicy.FDSP_MEDIA_POLICY_HDD;
				break;
			case SSD_ONLY:
				fdspPolicy = FDSP_MediaPolicy.FDSP_MEDIA_POLICY_SSD;
				break;
			default: 
				fdspPolicy = FDSP_MediaPolicy.FDSP_MEDIA_POLICY_UNSET;
				break;
		}
		
		return fdspPolicy;
	}
	
}

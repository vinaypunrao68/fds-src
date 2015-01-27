package com.formationds.commons.model;

import com.formationds.commons.model.type.ManagerType;

/**
 * This class is meant to provide future flexibility for this
 * particular type of service should it need to diverge from the 
 * base class
 * 
 * @author nate
 *
 */
public class PlatformManagerService extends Service {

	/**
	 * 
	 */
	private static final long serialVersionUID = -3037468436245114722L;

	public PlatformManagerService(){
		
		super( ManagerType.FDSP_PLATFORM );
	}
}

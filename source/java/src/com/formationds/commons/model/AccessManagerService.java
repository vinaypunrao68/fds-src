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
public class AccessManagerService extends Service {

	/**
	 * 
	 */
	private static final long serialVersionUID = 5217019672357952871L;

	public AccessManagerService(){
		
		super( ManagerType.FDSP_STOR_HVISOR );
	}
}

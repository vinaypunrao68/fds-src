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
public class StorageManagerService extends Service {

	/**
	 * 
	 */
	private static final long serialVersionUID = 7888039061466816350L;

	public StorageManagerService() {
		
		super( ManagerType.FDSP_STOR_MGR );
	}
	
}

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
public class OrchestrationManagerService extends Service {

	/**
	 * 
	 */
	private static final long serialVersionUID = -7611844050067048017L;

	public OrchestrationManagerService(){
		
		super( ManagerType.FDSP_ORCH_MGR );
	}
}

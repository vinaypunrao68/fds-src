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
public class DataManagerService extends Service {

	/**
	 * 
	 */
	private static final long serialVersionUID = -6658877595817965262L;

	public DataManagerService(){
		
		super( ManagerType.FDSP_DATA_MGR );
	}
}

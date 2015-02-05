package com.formationds.om.helper;

public class ServiceMappingHelper {

	private static ServiceMappingHelper instance;
	
	private ServiceMappingHelper(){
		
	}
	
	public static ServiceMappingHelper getInstance() {
	
		if ( instance == null ) {
			instance = new ServiceMappingHelper();
		}
		
		return instance;
	}
	
	
}

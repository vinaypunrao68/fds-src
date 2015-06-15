package com.formationds.client.v08.model;

import org.junit.Test;

import com.formationds.commons.model.helper.ObjectModelHelper;

public class TenantTest {

	@Test
	public void testBasicTenantConversion(){
		
		Tenant tenant = new Tenant( 99L, "TrashHeap" );
		
		String jsonString = ObjectModelHelper.toJSON( tenant );
		
		Tenant newTenant = ObjectModelHelper.toObject( jsonString, Tenant.class );
		
		assert newTenant.getId().equals( tenant.getId() );
		assert tenant.getName().equals( tenant.getName() );
	}
}

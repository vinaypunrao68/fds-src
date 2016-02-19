package com.formationds.client.v08.model;

import org.junit.Test;

import com.formationds.client.v08.model.Domain.DomainState;
import com.formationds.commons.model.helper.ObjectModelHelper;

public class DomainTest {

	@Test
	public void testDomainConversion(){
		
		Domain domain = new Domain( 9, "FDS", "Boulder", DomainState.UP );
		
		String jsonString = ObjectModelHelper.toJSON( domain );
		
		Domain newDomain = ObjectModelHelper.toObject( jsonString, Domain.class ); 
		
		assert newDomain.getId().equals( domain.getId() );
		assert newDomain.getName().equals( domain.getName() );
		assert newDomain.getSite().equals( domain.getSite() );
	}
}

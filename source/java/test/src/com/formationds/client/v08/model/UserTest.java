package com.formationds.client.v08.model;

import org.junit.Test;

import com.formationds.commons.model.helper.ObjectModelHelper;

public class UserTest {

	@Test
	public void testBasicUserConversion(){
		
		User user = new User( 5L, "justin", 0L, null );
		
		String json = ObjectModelHelper.toJSON( user );
		
		User newUser = ObjectModelHelper.toObject( json, User.class );
		
		assert newUser.getId().equals( user.getId() );
		assert newUser.getName().equals( user.getName() );
		
	}
	
}

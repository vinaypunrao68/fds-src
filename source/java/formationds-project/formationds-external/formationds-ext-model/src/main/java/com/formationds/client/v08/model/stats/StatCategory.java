package com.formationds.client.v08.model.stats;

import java.util.ArrayList;
import java.util.List;

public enum StatCategory {

	PERFORMANCE( new ArrayList<SUB_CATEGORY>() ),
	CAPACITY( new ArrayList<SUB_CATEGORY>() );
	
	public enum SUB_CATEGORY{
		NONE
	};
	
	private final List<SUB_CATEGORY> subcategories;
	
	private StatCategory( List<SUB_CATEGORY> subcategories ){
		this.subcategories = subcategories;
	}
	
	public List<SUB_CATEGORY> getSubCategories(){
		return this.subcategories;
	}
}

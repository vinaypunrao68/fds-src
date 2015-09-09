package com.formationds.client.v08.model.stats;

public class ContextDef {

	private ContextType contextType;
	private Long contextId;
	
	public ContextDef(){}
	
	public ContextDef( ContextType aType, Long anId ){
		contextType = aType;
		contextId = anId;
	}
	
	public ContextType getContextType() {
		return contextType;
	}
	
	public void setContextType( ContextType contextType ) {
		this.contextType = contextType;
	}
	
	public Long getContextId() {
		return contextId;
	}
	
	public void setContextId( Long contextId ) {
		this.contextId = contextId;
	}
}

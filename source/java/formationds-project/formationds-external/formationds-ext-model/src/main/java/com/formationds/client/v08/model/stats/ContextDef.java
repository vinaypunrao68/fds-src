package com.formationds.client.v08.model.stats;

import org.json.JSONObject;

public class ContextDef {

	private ContextType contextType;
	private Long contextId;
	
	private static final String CONTEXT_TYPE = "contextType";
	private static final String CONTEXT_ID = "contextId";
	
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
	
	public JSONObject toJsonObject(){
		
		JSONObject json = new JSONObject();
		
		json.put( CONTEXT_TYPE, getContextType().name() );
		json.put( CONTEXT_ID, getContextId() );
		
		return json;
	}
	
	@Override
	public boolean equals( Object obj) {
	
		if ( !(obj instanceof ContextDef) ){
			return false;
		}
		
		ContextDef def = (ContextDef)obj;
		
		return ( def.getContextId().equals( getContextId() ) && def.getContextType().equals( getContextType() ) );
	}
	
	public String toJson(){
		return toJsonObject().toString();
	}
	
	public static ContextDef buildFromJson( String json ){
		
		JSONObject jsonO = new JSONObject( json );
		ContextDef def = new ContextDef( ContextType.valueOf( jsonO.getString( CONTEXT_TYPE ) ), jsonO.getLong( CONTEXT_ID ) );
		
		return def;
	}
}

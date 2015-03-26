package com.formationds.om.repository.influxdb;

import java.util.Collection;
import java.util.List;

import com.formationds.commons.crud.EntityPersistListener;
import com.formationds.commons.events.FirebreakType;
import com.formationds.commons.model.Volume;
import com.formationds.commons.model.entity.Event;
import com.formationds.commons.model.entity.FirebreakEvent;
import com.formationds.commons.model.entity.UserActivityEvent;
import com.formationds.om.repository.EventRepository;
import com.formationds.om.repository.query.QueryCriteria;

public class InfluxEventRepository extends InfluxRepository<Event, Long> implements EventRepository {

	protected InfluxEventRepository(String url, String adminUser,
			char[] adminCredentials) {
		super(url, adminUser, adminCredentials);
		// TODO Auto-generated constructor stub
	}

	@Override
	public Class<Event> getEntityClass() {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public void addEntityPersistListener(EntityPersistListener<Event> l) {
		// TODO Auto-generated method stub

	}

	@Override
	public void removeEntityPersistListener(EntityPersistListener<Event> l) {
		// TODO Auto-generated method stub

	}

	@Override
	public long countAll() {
		// TODO Auto-generated method stub
		return 0;
	}

	@Override
	public long countAllBy(Event entity) {
		// TODO Auto-generated method stub
		return 0;
	}

	@Override
	public Event save(Event entity) {
		// TODO Auto-generated method stub
		return null;
	}
	
    /**
     * Method to create a string from the query object that matches influx format
     * 
     * @param queryCriteria
     * @return
     */
    protected String formulateQueryString( QueryCriteria queryCriteria ){
    	
    	StringBuilder sb = new StringBuilder();
        
    	String prefix = SELECT + " * " + FROM + " " + getEntityName();
    	sb.append( prefix );
    
    	if ( queryCriteria.getRange() != null && 
    			queryCriteria.getContexts() != null && queryCriteria.getContexts().size() > 0 ) {
    		
    		sb.append( " " + WHERE );
    	}
    	
    	// do time range
    	if ( queryCriteria.getRange() != null ){
    	
	    	String time = " ( " + getTimestampColumnName() + " >= " + queryCriteria.getRange().getStart() + " " + AND + 
	    			" " + getTimestampColumnName() + " <= " + queryCriteria.getRange().getEnd() + " ) ";
	    	
	    	sb.append( time );
    	}
    	
    	return sb.toString();
    }

	@Override
	public List<? extends Event> query(QueryCriteria queryCriteria) {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public void delete(Event entity) {
		// TODO Auto-generated method stub

	}

	@Override
	public void close() {
		// TODO Auto-generated method stub

	}

	@Override
	public List<UserActivityEvent> queryTenantUsers(QueryCriteria queryCriteria,
			List<Long> tenantUsers) {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public FirebreakEvent findLatestFirebreak(Volume v, FirebreakType type) {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	protected <R extends Event> R doPersist(R entity) {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	protected <R extends Event> List<R> doPersist(Collection<R> entities) {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	protected void doDelete(Event entity) {
		// TODO Auto-generated method stub
		
	}

}

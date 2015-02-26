/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository.query.builder;

import com.formationds.commons.model.DateRange;
import com.formationds.commons.model.abs.Context;
import com.formationds.om.repository.query.OrderBy;
import com.formationds.om.repository.query.QueryCriteria;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.persistence.EntityManager;
import javax.persistence.TypedQuery;
import javax.persistence.criteria.*;

import java.lang.reflect.ParameterizedType;
import java.util.ArrayList;
import java.util.List;

/**
 * The criteria query builder constructs a JPA TypedQuery for the specified entity type.
 * <p/>
 * It may be overridden in subclasses to provide builder methods to add additional predicates to the query.
 *
 * @param <T>
 */
@SuppressWarnings( "UnusedDeclaration" )
public class CriteriaQueryBuilder<T> {

    private static final Logger logger = LoggerFactory.getLogger(CriteriaQueryBuilder.class);

    protected static final String VALUE = "value";

    protected final EntityManager em;
    protected final CriteriaBuilder cb;
    protected final CriteriaQuery<T> cq;
    protected final Root<T> from;
    protected List<Predicate> orPredicates;
    protected List<Predicate> andPredicates;
    protected List<Order> orderPrecedence;
    private Integer maxResults;
    private Integer firstResult;

    private final String timestampField;
    private final String contextName;

    /**
     * @param entityManager the {@link javax.persistence.EntityManager}
     * @param contextName the name of the context to use in queries
     */
    public CriteriaQueryBuilder( final EntityManager entityManager,
                                 final String timestampField,
                                 final String contextName ) {
        this.em = entityManager;
        this.cb = em.getCriteriaBuilder();
        this.cq = cb.createQuery( getEntityClass() );
        this.from = cq.from( getEntityClass() );
        this.andPredicates = new ArrayList<>();
        this.orPredicates = new ArrayList<>();

        this.timestampField = timestampField;
        this.contextName = contextName;
    }

    /**
     * @return the context name
     */
    public String getContextName() { return contextName; }

    /**
     * @return the context name
     */
    public String getTimestampField() { return timestampField; }
    
    /**
     * @return the list of order objects
     */
    protected List<Order> getOrderPrecedence() {
    	
    	if ( orderPrecedence == null ) {
    		orderPrecedence = new ArrayList<Order>();
    	}
    	
    	return orderPrecedence;
    }

    /**
     * @param dateRange the {@link com.formationds.commons.model.DateRange} representing the search date range window
     *
     * @return Returns {@link MetricCriteriaQueryBuilder}
     */
    @SuppressWarnings( "unchecked" )
    public <CQ extends CriteriaQueryBuilder> CQ withDateRange( final DateRange dateRange ) {
        if( dateRange != null ) {
            final Path<Long> timestamp = from.get( getTimestampField() );
            logger.trace( String.format( "START:: %d END:: %d ",
                                         dateRange.getStart(),
                                         dateRange.getEnd() ) );
            Predicate predicate = null;
            if( ( dateRange.getStart() != null ) &&
                        ( dateRange.getEnd() != null ) ) {
                predicate = cb.and( cb.ge( timestamp, dateRange.getStart() ),
                                    cb.le( timestamp, dateRange.getEnd() ) );
            } else if( dateRange.getStart() != null ) {
                predicate = cb.ge( from.<Long>get( getTimestampField() ),
                                   dateRange.getStart() );
            } else if( dateRange.getEnd() != null ) {
                predicate = cb.le( from.<Long>get( getTimestampField() ),
                                   dateRange.getEnd() );
            }

            if( predicate != null ) {
                andPredicates.add( predicate );
            }
        }

        return (CQ)this;
    }
    
    public <CQ extends CriteriaQueryBuilder> CQ withOrderInstructions( final List<OrderBy> orders ){
   
    	orders.stream().forEach( order -> {
        	
    		logger.trace( String.format( "Order criteria: %s ASCENDING:: %b ", 
        			order.getFieldName(), 
        			order.isAscending() ) );
    		
    		Order javaxOrder = null;
    		
        	if ( order.isAscending() ){
        		javaxOrder = cb.asc( from.get( order.getFieldName() ) );
        	}
        	else {
        		javaxOrder = cb.desc( from.get( order.getFieldName() ) );        		
        	}
        	
        	getOrderPrecedence().add( javaxOrder );
    	});
    	
    	return (CQ)this;
    }

    /**
     * @param maxResults the {@code int} representing the maximum result set size
     *
     * @return Returns {@link CriteriaQueryBuilder}
     */
    @SuppressWarnings( "unchecked" )
    public <CQ extends CriteriaQueryBuilder> CQ maxResults( final int maxResults ) {
        this.maxResults = maxResults;
        return (CQ)this;
    }

    /**
     * @param firstResult the {@code int} representing the starting results set row id
     *
     * @return Returns {@link CriteriaQueryBuilder}
     */
    @SuppressWarnings( "unchecked" )
    public <CQ extends CriteriaQueryBuilder> CQ firstResult( final int firstResult ) {
        this.firstResult = firstResult;
        return (CQ)this;
    }

    /**
     * @param searchCriteria the {@link com.formationds.om.repository.query.MetricQueryCriteria}
     *
     * @return Returns {@link MetricCriteriaQueryBuilder}
     */
    @SuppressWarnings( "unchecked" )
    public <CQ extends CriteriaQueryBuilder> CQ searchFor(final QueryCriteria searchCriteria ) {

        if ( searchCriteria.getRange() != null ) {
            this.withDateRange( searchCriteria.getRange() );
        }

        this.withContexts( searchCriteria.getContexts() );

        if ( searchCriteria.getFirstPoint() != null ) {
            this.firstResult( searchCriteria.getFirstPoint().intValue() );
        }

        if ( searchCriteria.getPoints() != null ) {
            this.maxResults( searchCriteria.getPoints() );
        }
        
        if ( !searchCriteria.getOrderBy().isEmpty() ) {
        	this.withOrderInstructions( searchCriteria.getOrderBy() );
        }

        return (CQ)this;
    }

    /**
     * @param contexts the {@link List} of {@link com.formationds.commons.model.abs.Context}
     *
     * @return Returns {@link MetricCriteriaQueryBuilder}
     */
    @SuppressWarnings( "unchecked" )
    public <CQ extends CriteriaQueryBuilder> CQ withContexts( final List<Context> contexts ) {

        if( contexts != null && !contexts.isEmpty() ) {

            final List<String> in = new ArrayList<>();
            for( final Context c : contexts ) {

                in.add( c.getContext() );

            }

            final Expression<String> expression = from.get( getContextName() );
            andPredicates.add(expression.in(in.toArray(new String[in.size()])));
        }

        return (CQ)this;
    }

    /**
     * @return Returns a {@link List} of the entities matching the query criteria
     */
    public List<T> resultsList() {
        if( firstResult != null && maxResults != null ) {
            return build().setFirstResult( firstResult )
                          .setMaxResults( maxResults )
                          .getResultList();
        } else if( firstResult != null ) {
            return build().setFirstResult( firstResult )
                          .getResultList();
        } else if( maxResults != null ) {
            return build().setMaxResults( maxResults )
                          .getResultList();
        }

        return build().getResultList();
    }

    /**
     * @return Returns the matching entity
     */
    public T singleResult() {
        if( firstResult != null && maxResults != null ) {
            return build().setFirstResult( firstResult )
                          .setMaxResults( maxResults )
                          .getSingleResult();
        } else if( firstResult != null ) {
            return build().setFirstResult( firstResult )
                          .getSingleResult();
        } else if( maxResults != null ) {
            return build().setMaxResults( maxResults )
                          .getSingleResult();
        }

        return build().getSingleResult();
    }


    /**
     * @return the class
     */
    @SuppressWarnings("unchecked")
    public Class<T> getEntityClass() {
        return ( Class<T> ) ( (ParameterizedType) getClass()
            .getGenericSuperclass() )
            .getActualTypeArguments()[ 0 ];
    }

    /**
     * @return Returns the {@link TypedQuery} representing the query for the entity
     */
    public TypedQuery<T> build() {
        logger.trace( "AND::{} OR::{}", andPredicates, orPredicates );
        
        CriteriaQuery<T> criteriaQuery = null;
        
        // handle the predicates.  Ordering is handled once below
        if( !andPredicates.isEmpty() && !orPredicates.isEmpty() ) {
        	
        	criteriaQuery = cq.select( from )
                               .where(
                                   cb.and(
                                       andPredicates.toArray(
                                           new Predicate[ andPredicates.size() ] ) ),
                                   cb.or(
                                       orPredicates.toArray(
                                           new Predicate[ orPredicates.size() ] ) )
                                     );
        }
        else if( andPredicates.isEmpty() && !orPredicates.isEmpty() ) {
        	criteriaQuery =  cq.select( from )
                               .where(
                                   cb.or(
                                       orPredicates.toArray(
                                           new Predicate[ orPredicates.size() ] ) )
                                     );
        }
        else if( !andPredicates.isEmpty() ) {
        	criteriaQuery = cq.select( from )
                               .where(
                                   cb.and(
                                       andPredicates.toArray(
                                           new Predicate[ andPredicates.size() ] ) )
                                     );
        }
        else {
        	criteriaQuery = cq.select( from );
        }
        
        // setting the default ordering if none was specified
        if ( getOrderPrecedence().isEmpty() ) {
        	criteriaQuery = cq.orderBy( cb.asc( from.get( getTimestampField() ) ), 
        								cb.asc( from.get( getContextName() ) ) );
        }
        // otherwise we use the list of specified order precedence
        else {
        	criteriaQuery = cq.orderBy( getOrderPrecedence() );
        }
        
        return em.createQuery( criteriaQuery );
    }

    @SuppressWarnings( "unchecked" )
    protected <CQ extends CriteriaQueryBuilder> CQ and(Predicate p) {
        andPredicates.add(p);
        return (CQ)this;
    }

    @SuppressWarnings( "unchecked" )
    protected <CQ extends CriteriaQueryBuilder> CQ or(Predicate p) {
        orPredicates.add(p);
        return (CQ)this;
    }
}

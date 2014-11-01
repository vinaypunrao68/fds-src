/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository;

import com.formationds.commons.crud.JDORepository;
import com.formationds.commons.model.Events;
import com.formationds.commons.model.entity.Event;
import com.formationds.om.repository.query.QueryCriteria;
import com.formationds.om.repository.query.builder.CriteriaQueryBuilder;

import javax.jdo.Query;
import javax.persistence.EntityManager;
import javax.persistence.TypedQuery;
import java.util.List;

// TODO: not implemented
public class EventRepository extends JDORepository<Event, Long, Events, QueryCriteria> {

    // TODO: default should be based on fdsroot (is this available somewhere?)
    private static final String DBNAME = "/fds/var/db/events.odb";

    /**
     * default constructor
     */
    public EventRepository() {
        this(DBNAME);
    }

    /**
     * @param dbName the {@link String} representing the name and location of the
     *               repository
     */
    public EventRepository( final String dbName ) {
        super();
        initialize(dbName);
    }

    @Override
    public Event findById(Long id) {
        final Query query = manager().newQuery(Event.class, "id == :id");
        query.setUnique( true );
        try {
            return ( Event ) query.execute( id );
        } finally {
            query.closeAll();
        }
    }

    // TODO: what is this API asking for?  Count all events by what? non-null fields of the entity like JPA QueryByExample?
    @Override
    public long countAllBy(Event entity) {
        return 0;
    }

    @Override
    public Events query(QueryCriteria queryCriteria) {
        // TODO: not sure why the build() method doesn't set first/max results but it doesn't currently
        final List<Event> results =
                (new EventCriteriaQueryBuilder(entity()).searchFor(queryCriteria).resultsList());

        return new Events(results);
    }

    private static class EventCriteriaQueryBuilder extends CriteriaQueryBuilder<Event> {

        // TODO: how to support multiple contexts (category and severity)?
        private static final String CONTEXT = "category";
        private static final String TIMESTAMP = "initialTimestamp";

        EventCriteriaQueryBuilder(EntityManager em) {
            super(em, TIMESTAMP, CONTEXT);
        }
    }

}

/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository;

import com.formationds.commons.crud.JDORepository;
import com.formationds.commons.events.FirebreakType;
import com.formationds.commons.model.Events;
import com.formationds.commons.model.Volume;
import com.formationds.commons.model.builder.DateRangeBuilder;
import com.formationds.commons.model.entity.Event;
import com.formationds.commons.model.entity.FirebreakEvent;
import com.formationds.commons.model.entity.UserActivityEvent;
import com.formationds.om.helper.SingletonConfiguration;
import com.formationds.om.repository.query.QueryCriteria;
import com.formationds.om.repository.query.builder.CriteriaQueryBuilder;

import javax.jdo.Query;
import javax.persistence.EntityManager;
import javax.persistence.criteria.Expression;
import java.io.File;
import java.sql.Timestamp;
import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 *
 */
public class EventRepository extends JDORepository<Event, Long, Events, QueryCriteria> {

    private static final String DBNAME = "var/db/events.odb";

    /**
     * default constructor
     */
    public EventRepository() {
        this( SingletonConfiguration.instance().getConfig().getFdsRoot() +
                  File.separator +
                  DBNAME );
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
        final List<Event> results;

        EventCriteriaQueryBuilder tq = new EventCriteriaQueryBuilder(entity()).searchFor(queryCriteria);
        results = tq.resultsList();
        return new Events(results);
    }

    public Events queryTenantUsers(QueryCriteria queryCriteria, List<Long> tenantUsers) {
        final List<? extends Event> results;

        UserEventCriteriaQueryBuilder tq =
                new UserEventCriteriaQueryBuilder(entity()).usersIn(tenantUsers)
                                                       .searchFor(queryCriteria);
        results = tq.resultsList();
        return new Events(results);
    }

    /**
     *
     * @param v
     * @return
     */
    public FirebreakEvent findLatestFirebreak(Volume v) {
        FirebreakEventCriteriaQueryBuilder cb = new FirebreakEventCriteriaQueryBuilder(entity());

        Instant oneDayAgo = Instant.now().minus(Duration.ofDays(1));
        Timestamp tsOneDayAgo = new Timestamp(oneDayAgo.toEpochMilli());
        cb.volumesByName(v.getName()).withDateRange(new DateRangeBuilder(tsOneDayAgo, null).build());
        List<FirebreakEvent> r = cb.build().getResultList();
        if (r.isEmpty()) return null;

        r.sort((f, fp) -> f.getInitialTimestamp().compareTo(fp.getInitialTimestamp()));
        return r.get(0);
    }

    private static class EventCriteriaQueryBuilder extends CriteriaQueryBuilder<Event> {

        // TODO: how to support multiple contexts (category and severity)?
        private static final String CONTEXT = "category";
        private static final String TIMESTAMP = "initialTimestamp";

        EventCriteriaQueryBuilder(EntityManager em) {
            super(em, TIMESTAMP, CONTEXT);
        }
    }

    private static class UserEventCriteriaQueryBuilder extends CriteriaQueryBuilder<UserActivityEvent> {

        // TODO: how to support multiple contexts (category and severity)?
        private static final String CONTEXT = "category";
        private static final String TIMESTAMP = "initialTimestamp";
        static final String USER_ID = "userId";

        UserEventCriteriaQueryBuilder(EntityManager em) {
            super(em, TIMESTAMP, CONTEXT);
        }

        protected UserEventCriteriaQueryBuilder usersIn(List<Long> in) {
            final Expression<String> expression = from.get( USER_ID );
            super.and(expression.in(in.toArray(new Long[in.size()])));
            return this;
        }
    }

    private static class FirebreakEventCriteriaQueryBuilder extends CriteriaQueryBuilder<FirebreakEvent> {

        // TODO: how to support multiple contexts (category and severity)?
        private static final String CONTEXT = "category";
        private static final String TIMESTAMP = "initialTimestamp";
        static final String VOLID = "volumeId";
        static final String VOLNAME = "volumeName";
        static final String FBTYPE = "firebreakType";

        FirebreakEventCriteriaQueryBuilder(EntityManager em) {
            super(em, TIMESTAMP, CONTEXT);
        }

        protected FirebreakEventCriteriaQueryBuilder volumesById(String... in) {
            return volumesById((in != null ? Arrays.asList(in) : new ArrayList<>()));
        }

        protected FirebreakEventCriteriaQueryBuilder volumesById(List<String> in) {
            final Expression<String> expression = from.get( VOLID );
            super.and(expression.in(in.toArray(new String[in.size()])));
            return this;
        }

        protected FirebreakEventCriteriaQueryBuilder volumesByName(String... in) {
            return volumesByName((in != null ? Arrays.asList(in) : new ArrayList<>()));
        }

        protected FirebreakEventCriteriaQueryBuilder volumesByName(List<String> in) {
            final Expression<String> expression = from.get( VOLNAME );
            super.and(expression.in(in.toArray(new String[in.size()])));
            return this;
        }

        protected FirebreakEventCriteriaQueryBuilder fbType(FirebreakType type) {
            final Expression<String> expression = from.get( FBTYPE );
            super.and( cb.equal(expression, type) );
            return this;
        }
    }
}

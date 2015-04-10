/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.repository;

import com.formationds.commons.crud.JDORepository;
import com.formationds.commons.events.FirebreakType;
import com.formationds.commons.model.Volume;
import com.formationds.commons.model.builder.DateRangeBuilder;
import com.formationds.commons.model.entity.Event;
import com.formationds.commons.model.entity.FirebreakEvent;
import com.formationds.commons.model.entity.UserActivityEvent;
import com.formationds.om.helper.SingletonConfiguration;
import com.formationds.om.repository.query.QueryCriteria;
import com.formationds.om.repository.query.builder.CriteriaQueryBuilder;

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
public class JDOEventRepository extends JDORepository<Event, Long> implements EventRepository {

    private static final String DBNAME = "var/db/events.odb";

    /**
     * default constructor
     */
    public JDOEventRepository() {
        this( SingletonConfiguration.instance().getConfig().getFdsRoot() +
                  File.separator +
                  DBNAME );
    }

    /**
     * @param dbName the {@link String} representing the name and location of the
     *               repository
     */
    public JDOEventRepository( final String dbName ) {
        super();
        initialize(dbName);
    }

    // TODO: what is this API asking for?  Count all events by what? non-null fields of the entity like JPA QueryByExample?
    @Override
    public long countAllBy(Event entity) {
        return 0;
    }

    // NOTE: this shouldn't be strictly necessary since JDORepository implements via default
    // implementation in CRUDRepository.  The declaration in EventRepository with the same signature
    // is confusing the intellij compilation
    @Override
    public List<Event> save( Event... entities ) {
        return super.save( entities );
    }

    @Override
    public List<? extends Event> query( QueryCriteria queryCriteria ) {
        EntityManager em = newEntityManager();
        try {
            EventCriteriaQueryBuilder tq = new EventCriteriaQueryBuilder(em).searchFor(queryCriteria);
            return tq.resultsList();
        } finally {
            em.close();
        }
    }

    @Override
    public List<UserActivityEvent> queryTenantUsers( QueryCriteria queryCriteria, List<Long> tenantUsers ) {
        EntityManager em = newEntityManager();
        try {
            UserEventCriteriaQueryBuilder tq =
            new UserEventCriteriaQueryBuilder(em).usersIn(tenantUsers)
                                                 .searchFor(queryCriteria);
            return tq.resultsList();
        } finally {
            em.close();
        }
    }

    /**
     * Find the latest firebreak on the specified volume and type.
     *
     * @param v the volume
     * @param type the firebreak type
     *
     * @return the latest (active) firebreak event for the volume and firebreak type
     */
    @Override
    public FirebreakEvent findLatestFirebreak( Volume v, FirebreakType type ) {
        EntityManager em = newEntityManager();
        try {
            FirebreakEventCriteriaQueryBuilder cb = new FirebreakEventCriteriaQueryBuilder(em);
            Instant oneDayAgo = Instant.now().minus(Duration.ofDays(1));
            Timestamp tsOneDayAgo = new Timestamp(oneDayAgo.toEpochMilli());
            cb.volumeByName(v.getName())
              .volumeByFBType(type)
              .withDateRange(new DateRangeBuilder(tsOneDayAgo, null).build());

            List<FirebreakEvent> r = cb.build().getResultList();
            if (r.isEmpty()) return null;

            // TODO: reverse order in query and get(0)...
            return r.get(r.size()-1);
        } finally {
            em.close();
        }
    }

    public static class EventCriteriaQueryBuilder extends CriteriaQueryBuilder<Event> {

        // TODO: how to support multiple contexts (category and severity)?
        private static final String CONTEXT = "category";
        private static final String TIMESTAMP = "initialTimestamp";

        EventCriteriaQueryBuilder(EntityManager em) {
            super(em, TIMESTAMP, CONTEXT);
        }
    }

    public static class UserEventCriteriaQueryBuilder extends CriteriaQueryBuilder<UserActivityEvent> {

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

    public static class FirebreakEventCriteriaQueryBuilder extends CriteriaQueryBuilder<FirebreakEvent> {

        // TODO: how to support multiple contexts (category and severity)?
        private static final String CONTEXT = "category";
        private static final String TIMESTAMP = "initialTimestamp";
        static final String VOLID = "volumeId";
        static final String VOLNAME = "volumeName";
        static final String FBTYPE = "firebreakType";

        FirebreakEventCriteriaQueryBuilder(EntityManager em) {
            super(em, TIMESTAMP, CONTEXT);
        }

        protected FirebreakEventCriteriaQueryBuilder volumeById(String v) {
            final Expression<?> expression = from.get( VOLID );
            super.and( cb.equal(expression, v) );
            return this;
        }

        protected FirebreakEventCriteriaQueryBuilder volumeByName(String v) {
            final Expression<?> expression = from.get( VOLNAME );
            super.and( cb.equal(expression, v) );
            return this;
        }

        protected FirebreakEventCriteriaQueryBuilder volumeByFBType(FirebreakType t) {
            final Expression<?> expression = from.get( FBTYPE );
            super.and( cb.equal(expression, t) );
            return this;
        }

        protected FirebreakEventCriteriaQueryBuilder volumesById(String... in) {
            return volumesById((in != null ? Arrays.asList( in ) : new ArrayList<>()));
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

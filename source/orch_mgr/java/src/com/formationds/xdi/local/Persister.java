package com.formationds.xdi.local;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.hibernate.Session;
import org.hibernate.SessionFactory;
import org.hibernate.cfg.AnnotationConfiguration;
import org.hibernate.criterion.Restrictions;

public class Persister {

    private final SessionFactory sessionFactory;

    public Persister(String memoryDbName) {
        AnnotationConfiguration config = new AnnotationConfiguration()
                .addAnnotatedClass(Domain.class)
                .addAnnotatedClass(Volume.class);

        config.setProperty("hibernate.connection.driver_class", "org.h2.Driver");
        config.setProperty("hibernate.connection.url", "jdbc:h2:mem:" + memoryDbName);
        config.setProperty("hibernate.dialect", "org.hibernate.dialect.H2Dialect");
        config.setProperty("hibernate.connection.pool_size", "1");
        config.setProperty("hibernate.show_sql", "true");
        config.setProperty("hibernate.hbm2ddl.auto", "create");
        sessionFactory = config.buildSessionFactory();
    }

    public <T> T execute(Transaction<T> transaction) {
        Session session = sessionFactory.openSession();
        session.beginTransaction();
        T tee = transaction.run(session);
        session.getTransaction().commit();
        session.flush();
        session.close();
        return tee;
    }

    public  <T extends Persistent> T load(final Class<T> klass, final long id) {
        T tee = execute(new Transaction<T>() {
            @Override
            public T run(Session session) {
                T result = (T) session.createCriteria(klass)
                        .add(Restrictions.eq("id", id))
                        .uniqueResult();
                result.initialize();
                return result;
            }
        });
        return tee;
    }

    public <T extends Persistent> T update(final T tee) {
        return execute(new Transaction<T>() {
            @Override
            public T run(Session session) {
                session.update(tee);
                tee.initialize();
                return tee;
            }
        });
    }

    public <T extends Persistent> T create(final T tee) {
        return execute(new Transaction<T>() {
            @Override
            public T run(Session session) {
                session.save(tee);
                tee.initialize();
                return tee;
            }
        });
    }

    public <T extends Persistent> void delete(final T tee) {
        execute(new Transaction<Object>() {
            @Override
            public Object run(Session session) {
                session.delete(tee);
                return null;
            }
        });
    }
}

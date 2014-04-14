package com.formationds.xdi.local;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.hibernate.Criteria;
import org.hibernate.criterion.Restrictions;
import org.junit.Test;

import java.util.List;

import static org.junit.Assert.assertEquals;

public class PersisterTest {
    @Test
    public void testPersistence() throws Exception {
        Persister persister = new Persister("foo");
        Domain d = persister.create(new Domain("domain"));
        Volume v = persister.create(new Volume(d, "volume", 42));
        Volume thawed = persister.execute(session -> {
            return (Volume) session.createCriteria(Volume.class)
                    .createCriteria("domain")
                    .add(Restrictions.eq("name", "domain"))
                    .uniqueResult();
        });
        assertEquals("volume", thawed.getName());

    }
}

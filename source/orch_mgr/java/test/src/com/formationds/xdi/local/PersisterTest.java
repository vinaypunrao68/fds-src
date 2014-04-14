package com.formationds.xdi.local;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.junit.Test;

public class PersisterTest {
    @Test
    public void testPersistence() throws Exception {
        Persister persister = new Persister("foo");
        Domain domain = new Domain();
        domain.setName("foo");
        Domain d = persister.create(domain);
    }
}

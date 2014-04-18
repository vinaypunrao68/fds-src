package com.formationds.xdi.local;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.hibernate.criterion.Restrictions;
import org.junit.Test;

import java.util.List;

import static org.junit.Assert.assertEquals;

public class PersisterTest {
    @Test
    public void testPersistence() throws Exception {
        Persister persister = new Persister("foo");
        Domain d = persister.create(new Domain("domain"));
        persister.create(new Volume(d, "volume", 42));
        Volume v = persister.execute(session -> {
            return (Volume) session.createCriteria(Volume.class)
                    .createCriteria("domain")
                    .add(Restrictions.eq("name", "domain"))
                    .uniqueResult();
        });

        Blob blob = persister.create(new Blob(v, "blob"));
        persister.create(new Block(blob.getId(), 0, new byte[]{1, 2}));
        persister.create(new Block(blob.getId(), 1, new byte[]{3, 4}));

        Blob loaded = persister.load(Blob.class, blob.getId());
        List<Block> blocks = persister.execute(session -> session.createCriteria(Block.class)
                .add(Restrictions.eq("blobId", loaded.getId()))
                .list());
        assertEquals(2, blocks.size());
    }
}

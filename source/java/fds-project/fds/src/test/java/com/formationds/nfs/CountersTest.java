package com.formationds.nfs;

import org.junit.Test;

import java.util.Map;

import static org.junit.Assert.assertEquals;

public class CountersTest {
    @Test
    public void testIncrementDecrement() throws Exception {
        Counters counters = new Counters();
        assertEquals(0l, counters.get(Counters.Key.inodeCreate));
        counters.increment(Counters.Key.inodeCreate);
        assertEquals(1l, counters.get(Counters.Key.inodeCreate));
        assertEquals(1l, counters.reset(Counters.Key.inodeCreate));
        assertEquals(0l, counters.get(Counters.Key.inodeCreate));
        counters.increment(Counters.Key.inodeCreate);
        counters.decrement(Counters.Key.inodeCreate);
        assertEquals(0l, counters.get(Counters.Key.inodeCreate));

        counters.increment(Counters.Key.inodeCreate);
        Map<Counters.Key, Long> map = counters.harvest();
        assertEquals(1l, (long) map.get(Counters.Key.inodeCreate));
        assertEquals(0l, counters.get(Counters.Key.inodeCreate));
    }
}
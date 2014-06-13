package com.formationds.util;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.json.JSONArray;
import org.json.JSONObject;
import org.junit.Test;

import java.util.stream.Stream;

import static org.junit.Assert.assertEquals;

public class JsonArrayCollectorTest {
    @Test
    public void testCollectToJsonArray() {
        JSONArray array = Stream.of(1, 2, 3, 4)
                .map(i -> new JSONObject().put(Integer.toString(i), i))
                .collect(new JsonArrayCollector());
        assertEquals("[{\"1\":1},{\"2\":2},{\"3\":3},{\"4\":4}]", array.toString());
    }
}

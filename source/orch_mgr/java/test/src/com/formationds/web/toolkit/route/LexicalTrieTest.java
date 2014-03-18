package com.formationds.web.toolkit.route;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.junit.Test;

import java.util.Map;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

public class LexicalTrieTest {
    @Test
    public void testIndex() {
        LexicalTrie<Integer> trie = new Root<Integer>().put("/foo/:panda/hello/:human", 42);
        QueryResult result = trie.find("/foo/42/hello/43");
        assertTrue(result.found());
        Map<String, String> matches = result.getMatches();
        assertEquals("42", matches.get("panda"));
        assertEquals("43", matches.get("human"));
        assertEquals(42, result.getValue());
    }
    @Test(expected= RuntimeException.class)
    public void testMultiplePatterns() {
        new Root<Integer>().put(":panda:poop", 42);
    }

    @Test(expected= RuntimeException.class)
    public void testIncompletePattern() {
        new Root<Integer>().put(":", 42);
    }
}

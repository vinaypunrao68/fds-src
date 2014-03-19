package com.formationds.web.toolkit.route;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.junit.Test;

import java.util.Map;

import static org.junit.Assert.*;

public class LexicalTrieTest {
    @Test
    public void testSimpleMatch() {
        LexicalTrie<Object> trie = LexicalTrie.newTrie();
        trie.put("a", 42);
        trie.put("aba", 44);
        trie.put("abb", 45);
        assertTrue(trie.find("a").found());
        assertFalse(trie.find("ab").found());
        assertTrue(trie.find("aba").found());
        assertTrue(trie.find("abb").found());
        assertFalse(trie.find("ccc").found());
        assertEquals(42, (int) trie.find("a").getValue());
        assertEquals(44, (int) trie.find("aba").getValue());
        assertEquals(45, (int) trie.find("abb").getValue());
    }
    @Test
    public void testIndex() {
        Root<Integer> trie = new Root<Integer>().put("/a/:panda/b/:human", 42);
        System.out.println(trie);
        QueryResult result = trie.find("/a/henry/b/bob");
        assertTrue(result.found());
        Map<String, String> matches = result.getMatches();
        assertEquals("henry", matches.get("panda"));
        assertEquals("bob", matches.get("human"));
        assertEquals(42, result.getValue());
    }

    @Test(expected = RuntimeException.class)
    public void testMultiplePatterns() {
        new Root<Integer>().put(":panda:hello", 42);
    }

    @Test(expected = RuntimeException.class)
    public void testIncompletePattern() {
        new Root<Integer>().put(":", 42);
    }

    @Test(expected = RuntimeException.class)
    public void testAmbiguousPatterns() {
        Root<Integer> trie = new Root<Integer>()
                .put("/a/:color/b/:metal", 42)
                .put("/a/:color", 43);
        QueryResult<Integer> goldResult = trie.find("/a/blue/b/gold");
        assertEquals(42, (int)goldResult.getValue());
        Map<String, String> matches = goldResult.getMatches();
        assertEquals("blue", matches.get("color"));
        assertEquals("gold", matches.get("metal"));

        QueryResult<Integer> redResult = trie.find("/a/red");
        assertEquals(43, (int)redResult.getValue());
        assertEquals("red", redResult.getMatches().get("color"));

    }
}

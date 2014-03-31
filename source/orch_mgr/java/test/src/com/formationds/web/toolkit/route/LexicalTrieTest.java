package com.formationds.web.toolkit.route;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.junit.Test;

import java.util.Map;

import static org.junit.Assert.*;

public class LexicalTrieTest {
    @Test
    public void testNestedCaptures() {
        LexicalTrie<Integer> trie = LexicalTrie.newTrie();
        trie.put("/a/:b/c/:d", 42);
        QueryResult<Integer> result = trie.find("/a/b/c/d");
        assertTrue(result.found());
        assertEquals(42, (int) result.getValue());
        assertEquals("b", result.getMatches().get("b"));
        assertEquals("d", result.getMatches().get("d"));
    }

    @Test
    public void testResolveCapture() {
        LexicalTrie<Integer> trie = LexicalTrie.newTrie();
        trie.put("/a", 42);
        trie.put("/a/:b", 43);
        trie.put("/a/:b/c", 44);
        System.out.println(trie);
        assertTrue(trie.find("/a").found());
        assertTrue(trie.find("/a/b").found());
        assertTrue(trie.find("/a/b/c").found());
    }

    @Test
    public void testSimpleMatch() {
        LexicalTrie<Integer> trie = LexicalTrie.newTrie();
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
        LexicalTrie<Integer> trie = LexicalTrie.<Integer>newTrie().put("/a/:panda/b/:human", 42);
        System.out.println(trie);
        QueryResult result = trie.find("/a/henry/b/bob");
        assertTrue(result.found());
        Map matches = result.getMatches();
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
}

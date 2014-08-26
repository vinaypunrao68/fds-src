package com.formationds.security;

import org.junit.Test;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

public class HashedPasswordTest {
    @Test
    public void testHashVerify() {
        HashedPassword hashedPassword = new HashedPassword();
        String hashed = hashedPassword.hash("hello");
        assertTrue(hashedPassword.verify(hashed, "hello"));
        assertFalse(hashedPassword.verify(hashed, "world"));
    }
}
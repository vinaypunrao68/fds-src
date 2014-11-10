package com.formationds.security;

import org.junit.Test;

import java.util.stream.IntStream;

import static org.junit.Assert.assertNotEquals;

public class FastUUIDTest {
    @Test
    public void testRandomness() {
        String first = FastUUID.randomUUID().toString();
        IntStream.range(0, 10000)
                .parallel()
                .mapToObj(i -> FastUUID.randomUUID().toString())
                .forEach(s -> assertNotEquals(s, first));
    }
}
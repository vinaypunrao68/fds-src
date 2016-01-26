package com.formationds.nfs;

import org.junit.Test;

import java.nio.ByteBuffer;
import java.util.Random;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;

public class FdsObjectTest {

    public static final int MAX_OBJECT_SIZE = 1024;

    @Test
    public void testAccessor() throws Exception {
        byte[] bytes = new byte[42];
        new Random().nextBytes(bytes);
        FdsObject fdsObject = FdsObject.wrap(bytes, MAX_OBJECT_SIZE);
        assertEquals(42, fdsObject.limit());
        assertEquals(MAX_OBJECT_SIZE, fdsObject.maxObjectSize());

        fdsObject.lock(o -> {
            assertEquals(42, o.limit());
            o.limit(43);
            assertEquals(43, o.limit());
            ByteBuffer bb = o.asByteBuffer();
            assertEquals(43, bb.limit());
            o.limit(42);
            assertArrayEquals(bytes, o.toByteArray());
            assertEquals(42, o.asByteBuffer().limit());
            return null;
        });
    }
}
package com.formationds.util.async;

import org.junit.Assert;
import org.junit.Test;

import java.nio.ByteBuffer;
import java.security.MessageDigest;

import static org.junit.Assert.*;

public class AsyncMessageDigestTest {

    @Test
    public void testBasic() throws Exception {
        MessageDigest md5 = MessageDigest.getInstance("MD5");
        AsyncMessageDigest asmd = new AsyncMessageDigest(md5);

        MessageDigest controlMd5 = MessageDigest.getInstance("MD5");
        ByteBuffer[] bufs = new ByteBuffer[] {
            ByteBuffer.wrap(new byte[]{1, 2, 3, 4, 5}),
            ByteBuffer.wrap(new byte[]{6, 7, 8, 9, 0}),
            ByteBuffer.wrap(new byte[]{1, 2, 3, 4, 5}),
        };

        for(ByteBuffer buf : bufs) {
            controlMd5.update(buf.slice());
        }

        for(ByteBuffer buf : bufs) {
            asmd.update(buf);
        }

        assertArrayEquals(controlMd5.digest(), asmd.get().get());
    }
}
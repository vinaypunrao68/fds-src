package com.formationds.util;

import org.junit.Test;

import java.io.ByteArrayInputStream;

import static org.junit.Assert.assertEquals;

public class ChunkingInputStreamTest {
    @Test
    public void testChunking() throws Exception {
        ByteArrayInputStream bais = new ByteArrayInputStream(new byte[] {0, 1, 2, 3, 4, 5, 6, 7});
        ChunkingInputStream in = new ChunkingInputStream(bais, 2);
        byte[] buf = new byte[8];
        int read = in.read(buf);
        assertEquals(2, read);
    }
}
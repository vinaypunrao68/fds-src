package com.formationds.util;

import junit.framework.TestCase;
import org.junit.Test;

import java.io.BufferedInputStream;
import java.io.ByteArrayInputStream;

import static org.junit.Assert.assertEquals;

public class ChunkingInputStreamTest {
    @Test
    public void testChunking() throws Exception {
        ByteArrayInputStream bais = new ByteArrayInputStream(new byte[] {0, 1, 2, 3, 4, 5, 6, 7});
        ChunkingInputStream in = new ChunkingInputStream(bais, 2);
        int read = in.read(new byte[8]);
        assertEquals(2, read);
    }

    @Test
    public void testJamesTheory() throws Exception {
        ByteArrayInputStream bais = new ByteArrayInputStream(new byte[] {0, 1, 2, 3, 4, 5, 6, 7});
        ChunkingInputStream chunky = new ChunkingInputStream(bais, 2);
        BufferedInputStream in = new BufferedInputStream(chunky, 8);
        int read = chunky.read(new byte[8]);
        assertEquals(4, read);

    }
}
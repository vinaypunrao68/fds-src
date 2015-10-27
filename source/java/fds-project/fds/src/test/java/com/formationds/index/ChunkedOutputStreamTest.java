package com.formationds.index;

import com.formationds.nfs.MemoryIoOps;
import com.formationds.nfs.TransactionalIo;
import org.apache.lucene.store.IndexInput;
import org.apache.lucene.store.IndexOutput;
import org.apache.lucene.store.OutputStreamIndexOutput;
import org.junit.Test;

import static org.junit.Assert.assertEquals;

public class ChunkedOutputStreamTest {
    @Test
    public void testWrite() throws Exception {
        TransactionalIo io = new TransactionalIo(new MemoryIoOps());
        int objectSize = 8;
        String DOMAIN = "foo";
        String VOLUME = "bar";
        String BLOB_NAME = "resource";

        ChunkedOutputStream out = new ChunkedOutputStream(io, "someResource", DOMAIN, VOLUME, BLOB_NAME, objectSize);
        IndexOutput indexOutput = new OutputStreamIndexOutput(BLOB_NAME, out, objectSize);
        String s = "Hello, world! This is truly a nice piece of litterature.";
        indexOutput.writeString(s);
        assertEquals(57, indexOutput.getFilePointer());
        indexOutput.close();

        IndexInput input = new FdsIndexInput(io, BLOB_NAME, DOMAIN, VOLUME, BLOB_NAME, objectSize);
        assertEquals(57, input.length());
        String string = input.readString();
        assertEquals(s, string);
    }
}
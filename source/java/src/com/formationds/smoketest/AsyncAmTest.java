package com.formationds.smoketest;

import com.formationds.apis.ErrorCode;
import com.formationds.apis.ObjectOffset;
import com.formationds.hadoop.FdsFileSystem;
import com.formationds.protocol.BlobDescriptor;
import com.google.common.collect.Maps;
import org.junit.Ignore;
import org.junit.Test;

import java.nio.ByteBuffer;
import java.util.UUID;

import static org.junit.Assert.assertEquals;


@Ignore
public class AsyncAmTest extends BaseAmTest {
    @Test
    public void testAsyncUpdateOnce() throws Exception {
        String blobName = UUID.randomUUID().toString();
        int length = 10;
        asyncAm.updateBlobOnce(FdsFileSystem.DOMAIN, volumeName, blobName, 1, ByteBuffer.allocate(length), length, new ObjectOffset(0), Maps.newHashMap()).get();
        BlobDescriptor blobDescriptor = asyncAm.statBlob(FdsFileSystem.DOMAIN, volumeName, blobName).get();
        assertEquals(length, blobDescriptor.getByteCount());
    }

    @Test
    public void testAsyncGetNonExistentBlob() throws Exception {
        String blobName = UUID.randomUUID().toString();
        assertFdsError(ErrorCode.MISSING_RESOURCE,
                () -> asyncAm.statBlob(FdsFileSystem.DOMAIN, volumeName, blobName).get());
    }
}

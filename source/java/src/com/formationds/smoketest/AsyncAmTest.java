package com.formationds.smoketest;

import org.junit.Ignore;
import org.junit.Test;


@Ignore
public class AsyncAmTest extends BaseAmTest {
    @Test
    public void testAsyncUpdateOnce() throws Exception {
//        String blobName = UUID.randomUUID().toString();
//        int length = 10;
//        asyncAm.updateBlobOnce(FdsFileSystem.DOMAIN, volumeName, blobName, 1, ByteBuffer.allocate(length), length, new ObjectOffset(0), Maps.newHashMap()).get();
//        BlobDescriptor blobDescriptor = asyncAm.statBlob(FdsFileSystem.DOMAIN, volumeName, blobName).get();
//        assertEquals(length, blobDescriptor.getByteCount());
    }

    @Test
    public void testAsyncGetNonExistentBlob() throws Exception {
//        String blobName = UUID.randomUUID().toString();
//        assertFdsError(ErrorCode.MISSING_RESOURCE,
//                () -> asyncAm.statBlob(FdsFileSystem.DOMAIN, volumeName, blobName).get());
    }
}

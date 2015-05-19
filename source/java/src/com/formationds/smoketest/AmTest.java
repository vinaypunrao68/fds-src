package com.formationds.smoketest;

import com.formationds.protocol.ApiException;
import com.formationds.protocol.ErrorCode;
import com.formationds.apis.*;
import com.formationds.hadoop.FdsFileSystem;
import com.formationds.protocol.BlobDescriptor;
import com.formationds.xdi.RealAsyncAm;
import com.formationds.xdi.XdiClientFactory;
import com.google.common.collect.Maps;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Ignore;
import org.junit.Test;

import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;

import static org.junit.Assert.assertEquals;

@Ignore
public class AmTest {
    private static XdiService.Iface amService;
    private static ConfigurationService.Iface configService;
    private String volumeName;
    public static final int OBJECT_SIZE = 1024 * 1024 * 2;
    public static final int MY_AM_RESPONSE_PORT = 9881;
    private static XdiClientFactory xdiCf;
    private static RealAsyncAm asyncAm;

    @Test
    public void testUpdateMetadata() throws Exception {
        Map<String, String> metadata = new HashMap<>();
        metadata.put("first", "firstValue");
        String blobName = UUID.randomUUID().toString();
        byte[] bytes = new byte[42];
        amService.updateBlobOnce(FdsFileSystem.DOMAIN, volumeName, blobName, 1, ByteBuffer.wrap(bytes), bytes.length, new ObjectOffset(0), metadata);

        // First statBlob after creation
        BlobDescriptor bd = amService.statBlob(FdsFileSystem.DOMAIN, volumeName, blobName);
        assertEquals(bd.getMetadata().get("first"), "firstValue");
        assertEquals(bytes.length, bd.getByteCount());

        // Update metadata
        metadata.put("second", "secondValue");
        TxDescriptor tx = amService.startBlobTx(FdsFileSystem.DOMAIN, volumeName, blobName, 0);
        amService.updateMetadata(FdsFileSystem.DOMAIN, volumeName, blobName, tx, metadata);
        amService.commitBlobTx(FdsFileSystem.DOMAIN, volumeName, blobName, tx);

        // Assert metadata visible and consistent
        bd = amService.statBlob(FdsFileSystem.DOMAIN, volumeName, blobName);
        assertEquals(bd.getMetadata().get("first"), "firstValue");
        assertEquals(bd.getMetadata().get("second"), "secondValue");
        // assertEquals(bytes.length, bd.getByteCount());
    }

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
        System.out.println("Blob name: " + blobName);
        try {
            asyncAm.statBlob(FdsFileSystem.DOMAIN, volumeName, blobName).get();
        } catch (ExecutionException e) {
            ApiException apiException = (ApiException) e.getCause();
            assertEquals(ErrorCode.MISSING_RESOURCE, apiException.getErrorCode());
        }
    }

    public AmTest() throws Exception {
    }

    @BeforeClass
    public static void setUpOnce() throws Exception {
        int pmPort = 7000;
        xdiCf = new XdiClientFactory();
        amService = xdiCf.remoteAmService("localhost", pmPort + 2988);
        configService = xdiCf.remoteOmService("localhost", 9090);
        asyncAm = new RealAsyncAm(xdiCf.remoteOnewayAm("localhost", pmPort + 1899), MY_AM_RESPONSE_PORT, 10, TimeUnit.MINUTES);
        asyncAm.start();
    }

    @Before
    public void setUp() throws Exception {
        volumeName = UUID.randomUUID().toString();
        configService.createVolume(FdsFileSystem.DOMAIN, volumeName, new VolumeSettings(OBJECT_SIZE, VolumeType.OBJECT, 0, 0, MediaPolicy.HDD_ONLY), 0);
    }

}

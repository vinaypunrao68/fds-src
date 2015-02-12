package com.formationds.smoketest;

import com.formationds.apis.*;
import com.formationds.protocol.BlobDescriptor;
import com.formationds.hadoop.FdsFileSystem;
import com.formationds.xdi.XdiClientFactory;
import org.junit.Before;
import org.junit.Test;

import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;
import java.util.UUID;

import static org.junit.Assert.assertEquals;

public class AmTest {
    private final AmService.Iface amService;
    private final ConfigurationService.Iface configService;
    private String volumeName;
    public static final int OBJECT_SIZE = 1024 * 1024 * 2;

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

    public AmTest() {
        XdiClientFactory xdiCf = new XdiClientFactory();
        amService = xdiCf.remoteAmService("localhost", 9988);
        configService = xdiCf.remoteOmService("localhost", 9090);
    }

    @Before
    public void setUp() throws Exception {
        volumeName = UUID.randomUUID().toString();
        configService.createVolume(FdsFileSystem.DOMAIN, volumeName, new VolumeSettings(OBJECT_SIZE, VolumeType.OBJECT, 0, 0, MediaPolicy.HDD_ONLY), 0);
    }
}

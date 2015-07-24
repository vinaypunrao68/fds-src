package com.formationds.smoketest;

import com.formationds.apis.MediaPolicy;
import com.formationds.apis.VolumeSettings;
import com.formationds.apis.VolumeType;
import com.formationds.commons.Fds;
import com.formationds.nfs.AmVfs;
import com.formationds.nfs.ExportResolver;
import com.formationds.nfs.NfsPath;
import com.formationds.util.ByteBufferUtility;
import com.formationds.util.ServerPortFinder;
import com.formationds.xdi.AsyncAm;
import com.formationds.xdi.RealAsyncAm;
import com.formationds.xdi.XdiClientFactory;
import com.formationds.xdi.XdiConfigurationApi;
import org.dcache.nfs.status.NoEntException;
import org.dcache.nfs.vfs.Inode;
import org.dcache.nfs.vfs.Stat;
import org.dcache.nfs.vfs.VirtualFileSystem;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Ignore;
import org.junit.Test;

import javax.security.auth.Subject;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.*;
import java.util.concurrent.TimeUnit;

import static org.junit.Assert.*;
import static org.mockito.Matchers.anyString;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;


@Ignore
public class NfsTest extends BaseAmTest {
    private static AmVfs amVfs;
    private static ExportResolver resolver;

    @Test
    public void testReadWriteMove() throws Exception {
        Inode rootInode = new NfsPath(volumeName, "/").asInode(Stat.Type.DIRECTORY, resolver);
        int length = 1024 * 1024 * 10;
        ByteBuffer someBytes = ByteBufferUtility.randomBytes(length);
        Inode sourceInode = amVfs.create(rootInode, Stat.Type.REGULAR, fileName, new Subject(), 0644);
        String destinationFile = UUID.randomUUID().toString();
        amVfs.write(sourceInode, someBytes.array(), 0, someBytes.remaining(), VirtualFileSystem.StabilityLevel.DATA_SYNC);
        Stat sourceMetadata = amVfs.getattr(sourceInode);

        byte[] readBuf = new byte[length];
        int read = amVfs.read(sourceInode, readBuf, 0, readBuf.length);
        assertEquals(length, read);
        assertArrayEquals(someBytes.array(), readBuf);
        assertTrue(amVfs.move(rootInode, fileName, rootInode, destinationFile));
        Inode destinationInode = amVfs.lookup(rootInode, destinationFile);
        Stat destinationMetadata = amVfs.getattr(destinationInode);
        assertNotEquals(sourceMetadata.getFileId(), destinationMetadata.getFileId());
        assertEquals(sourceMetadata.getMode(), destinationMetadata.getMode());
        assertEquals(sourceMetadata.getSize(), destinationMetadata.getSize());
        read = amVfs.read(destinationInode, readBuf, 0, readBuf.length);
        assertEquals(length, read);
        assertArrayEquals(someBytes.array(), readBuf);

        try {
            amVfs.getattr(sourceInode);
        } catch (NoEntException e) {
            return;
        }

        fail("Source file should have been removed");
    }

    @Test
    public void testNonSequentialParallelIo() throws Exception {
        NfsPath nfsPath = new NfsPath(volumeName, "/");
        int blockCount = 40;
        int length = OBJECT_SIZE * blockCount;
        byte[] bytes = new byte[length];
        new Random().nextBytes(bytes);
        List<Integer> chunks = new ArrayList<>(blockCount);
        for (int i = 0; i < blockCount; i++) {
            chunks.add(blockCount - i - 1);
        }
        Collections.shuffle(chunks);
        Inode inode = amVfs.create(nfsPath.asInode(Stat.Type.REGULAR, resolver), Stat.Type.REGULAR, fileName, new Subject(), 0644);

        chunks.stream()
                //.parallel()
                .forEach(i -> {
                    byte[] chunk = new byte[OBJECT_SIZE];
                    System.arraycopy(bytes, i * OBJECT_SIZE, chunk, 0, OBJECT_SIZE);
                    try {
                        amVfs.write(inode, chunk, i * OBJECT_SIZE, OBJECT_SIZE, VirtualFileSystem.StabilityLevel.DATA_SYNC);
                    } catch (IOException e) {
                        throw new RuntimeException(e);
                    }
                });
        byte[] readBuf = new byte[length];
        int read = amVfs.read(inode, readBuf, 0, readBuf.length);
        assertEquals(length, read);
        assertArrayEquals(bytes, readBuf);
        assertEquals((long) length, amVfs.getattr(inode).getSize());
    }

    private static XdiConfigurationApi config;
    private String volumeName;
    private String fileName;
    private static final int OBJECT_SIZE = 1024 * 1024 * 2;
    private static AsyncAm asyncAm;


    @BeforeClass
    public static void setUpOnce() throws Exception {
        XdiClientFactory xdiCf = new XdiClientFactory();
        config = new XdiConfigurationApi(xdiCf.remoteOmService(Fds.getFdsHost(), 9090));
        int responsePort = new ServerPortFinder().findPort("Async AM response", 10000);
        asyncAm = new RealAsyncAm(Fds.getFdsHost(), 8899, responsePort, 10, TimeUnit.SECONDS);
        asyncAm.start();
        resolver = mock(ExportResolver.class);
        when(resolver.exportId(anyString())).thenReturn(0);
        amVfs = new AmVfs(asyncAm, new XdiConfigurationApi(config), resolver);
    }

    @Before
    public void setUp() throws Exception {
        volumeName = UUID.randomUUID().toString();
        fileName = UUID.randomUUID().toString();
        config.createVolume(AmVfs.DOMAIN, volumeName, new VolumeSettings(OBJECT_SIZE, VolumeType.OBJECT, 0, 0, MediaPolicy.HDD_ONLY), 0);

    }
}

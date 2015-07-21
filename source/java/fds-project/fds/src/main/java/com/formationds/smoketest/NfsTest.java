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
import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.UUID;
import java.util.concurrent.TimeUnit;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.mockito.Matchers.anyString;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;


@Ignore
public class NfsTest extends BaseAmTest {

    private static AmVfs amVfs;
    private static ExportResolver resolver;

    @Test
    public void testAmVfs() throws Exception {
        NfsPath nfsPath = new NfsPath(volumeName, "/");
        int length = 1024 * 1024 * 10;
        ByteBuffer someBytes = ByteBufferUtility.randomBytes(length);
        Inode inode = amVfs.create(nfsPath.asInode(Stat.Type.REGULAR, resolver), Stat.Type.REGULAR, blobName, new Subject(), 0644);
        amVfs.write(inode, someBytes.array(), 0, someBytes.remaining(), VirtualFileSystem.StabilityLevel.DATA_SYNC);
        byte[] readBuf = new byte[length];
        int read = amVfs.read(inode, readBuf, 0, readBuf.length);
        assertEquals(length, read);
        assertArrayEquals(someBytes.array(), readBuf);
    }

    @Test
    public void testNonSequentialParallelIo() throws Exception {
        NfsPath nfsPath = new NfsPath(volumeName, "/");
        int blockSize = 1024 * 1024;
        int blockCount = 40;
        int length = blockSize * blockCount; // About 400M
        byte[] bytes = new byte[length];
        new Random().nextBytes(bytes);
        List<Integer> chunks = new ArrayList<>(blockCount);
        for (int i = 0; i < blockCount; i++) {
            chunks.add(blockCount - i - 1);
        }
//        Collections.shuffle(chunks);
        Inode inode = amVfs.create(nfsPath.asInode(Stat.Type.REGULAR, resolver), Stat.Type.REGULAR, blobName, new Subject(), 0644);

        chunks.stream()
                //.parallel()
                .forEach(i -> {
                    byte[] chunk = new byte[blockSize];
                    System.arraycopy(bytes, i * blockSize, chunk, 0, blockSize);
                    try {
                        amVfs.write(inode, chunk, i * blockSize, blockSize, VirtualFileSystem.StabilityLevel.DATA_SYNC);
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
    private String blobName;
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
        blobName = UUID.randomUUID().toString();
        config.createVolume(AmVfs.DOMAIN, volumeName, new VolumeSettings(OBJECT_SIZE, VolumeType.OBJECT, 0, 0, MediaPolicy.HDD_ONLY), 0);

    }
}

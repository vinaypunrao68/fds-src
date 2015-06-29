package com.formationds.smoketest;

import com.formationds.apis.*;
import com.formationds.commons.Fds;
import com.formationds.nfs.AmVfs;
import com.formationds.nfs.ExportResolver;
import com.formationds.nfs.NfsPath;
import com.formationds.util.ByteBufferUtility;
import com.formationds.util.ServerPortFinder;
import com.formationds.xdi.*;
import com.google.common.collect.Lists;
import org.dcache.nfs.ExportFile;
import org.dcache.nfs.FsExport;
import org.dcache.nfs.vfs.Inode;
import org.dcache.nfs.vfs.Stat;
import org.dcache.nfs.vfs.VirtualFileSystem;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Ignore;
import org.junit.Test;

import javax.security.auth.Subject;
import java.nio.ByteBuffer;
import java.util.UUID;
import java.util.concurrent.TimeUnit;

import static org.junit.Assert.*;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;


@Ignore
public class NfsTest extends BaseAmTest {
    @Test
    public void testAmVfs() throws Exception {
        ExportFile exportFile = mock(ExportFile.class);
        when(exportFile.getExports()).thenReturn(Lists.newArrayList(
                new FsExport.FsExportBuilder().build(volumeName)
        ));

        ExportResolver resolver = new ExportResolver(exportFile);
        AmVfs amVfs = new AmVfs(asyncAm, new XdiConfigurationApi(config), resolver);
        NfsPath nfsPath = new NfsPath(volumeName, "/");
        int length = 1024 * 1024 * 13;
        ByteBuffer someBytes = ByteBufferUtility.randomBytes(length);
        Inode inode = amVfs.create(nfsPath.asInode(Stat.Type.REGULAR, resolver), Stat.Type.REGULAR, blobName, new Subject(), 0644);
        amVfs.write(inode, someBytes.array(), 0, someBytes.remaining(), VirtualFileSystem.StabilityLevel.DATA_SYNC);
        byte[] readBuf = new byte[length];
        int read = amVfs.read(inode, readBuf, 0, readBuf.length);
        assertEquals(length, read);
        assertArrayEquals(someBytes.array(), readBuf);
    }

    private static XdiConfigurationApi config;
    private String volumeName;
    private String blobName;
    private static final int OBJECT_SIZE = 4096 * 32;
    private static AsyncAm asyncAm;


    @BeforeClass
    public static void setUpOnce() throws Exception {
        XdiClientFactory xdiCf = new XdiClientFactory();
        config = new XdiConfigurationApi(xdiCf.remoteOmService(Fds.getFdsHost(), 9090));
        int responsePort = new ServerPortFinder().findPort("Async AM response", 10000);
        asyncAm = new RealAsyncAm(Fds.getFdsHost(), 8899, responsePort, 10, TimeUnit.MINUTES);
        asyncAm.start();
    }

    @Before
    public void setUp() throws Exception {
        volumeName = UUID.randomUUID().toString();
        blobName = UUID.randomUUID().toString();
        config.createVolume(AmVfs.DOMAIN, volumeName, new VolumeSettings(OBJECT_SIZE, VolumeType.OBJECT, 0, 0, MediaPolicy.HDD_ONLY), 0);

    }
}
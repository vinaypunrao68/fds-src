package com.formationds.smoketest;

import com.formationds.apis.AmService;
import com.formationds.apis.ConfigurationService;
import com.formationds.apis.MediaPolicy;
import com.formationds.apis.VolumeSettings;
import com.formationds.apis.VolumeType;
import com.formationds.hadoop.FdsFileSystem;
import com.formationds.xdi.MemoryAmService;
import com.formationds.xdi.XdiClientFactory;

import org.apache.commons.compress.utils.IOUtils;
import org.apache.hadoop.fs.*;
import org.apache.hadoop.fs.FileSystem;
import org.junit.After;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Test;

import java.io.*;
import java.util.UUID;
import java.util.stream.IntStream;

import static org.junit.Assert.*;


@Ignore
public class HdfsSmokeTest {
    private final int OBJECT_SIZE = 1024 * 1024 * 2;
    private FdsFileSystem fileSystem;

    private static boolean isIntegrationTest() {
        return false;
    }

    @Test
    public void testListFiles() throws Exception {
        Path dir = new Path("/foo");
        fileSystem.mkdirs(dir);
        Path f = new Path("/foo/bar.txt");
        FSDataOutputStream stream = fileSystem.create(f);
        PrintWriter printWriter = new PrintWriter(new OutputStreamWriter(stream));
        printWriter.append("foo\n");
        printWriter.close();
        assertEquals(4, fileSystem.getFileStatus(f).getLen());
        assertEquals(1, fileSystem.listStatus(new Path("/foo")).length);
    }

    @Test
    public void testStat() throws Exception {
        Path f1 = new Path("/bar.txt");
        byte[] data1 = new byte[]{1, 2, 3, 4};
        byte[] data2 = new byte[]{4, 5, 6, 7};

        createWithContent(f1, data1);
        FileStatus status1 = fileSystem.getFileStatus(f1);

        Thread.sleep(100);

        createWithContent(f1, data2);
        FileStatus status2 = fileSystem.getFileStatus(f1);

        assertTrue(status1.getModificationTime() < status2.getModificationTime());
        assertFalse(status1.isDirectory());
        assertEquals(data1.length, status1.getLen());
        assertEquals(data2.length, status2.getLen());
        assertEquals(OBJECT_SIZE, status1.getBlockSize());
        assertEquals(fileSystem.getAbsolutePath(f1), status1.getPath());
    }

    @Test
    public void testStatLargeFile() throws Exception {
        Path p = new Path("/foobar.txt");
        int size = 11 * 1024 * 1024;
        byte[] contents = new byte[size];
        contents[size - 1] = 42;
        createWithContent(p, contents);
        FileStatus st = fileSystem.getFileStatus(p);
        assertEquals(size, (int) st.getLen());
        byte[] result = org.apache.commons.io.IOUtils.toByteArray(fileSystem.open(p));
        assertArrayEquals(contents, result);
    }

    @Test
    public void basicIoTest() throws Exception {
        byte[] data = {1, 2, 3, 4};

        Path f = new Path("/bar.txt");
        createWithContent(f, data);
        assertPathContent(f, data);
    }

    @Test
    public void deleteSingleTest() throws Exception {
        Path dir = new Path("/foop");
        fileSystem.mkdirs(dir);
        Path f = new Path("/foop/bar.txt");
        FSDataOutputStream str = fileSystem.create(f);
        str.close();
        assertTrue(fileSystem.exists(f));
        assertFalse(fileSystem.delete(f.getParent(), false));

        fileSystem.delete(f, false);
        assertTrue(!fileSystem.exists(f));
    }

    @Test
    public void deleteRecursiveTest() throws Exception {
        Path f = new Path("/bard");
        Path dissociated = new Path("/marts");

        Path sub1 = Path.mergePaths(f, new Path("/t1"));
        Path sub2 = Path.mergePaths(f, new Path("/t2"));

        Path dissociatedSub1 = Path.mergePaths(dissociated, new Path("/t3"));
        fileSystem.mkdirs(f);
        fileSystem.mkdirs(dissociated);
        touch(sub1);
        touch(sub2);
        touch(dissociatedSub1);

        assertTrue(fileSystem.exists(f));
        assertTrue(fileSystem.exists(sub1));
        assertTrue(fileSystem.exists(sub2));
        assertTrue(fileSystem.exists(dissociated));
        assertTrue(fileSystem.exists(dissociatedSub1));

        fileSystem.delete(f, true);

        assertTrue(!fileSystem.exists(f));
        assertTrue(!fileSystem.exists(sub1));
        assertTrue(!fileSystem.exists(sub2));
        assertTrue(fileSystem.exists(dissociated));
        assertTrue(fileSystem.exists(dissociatedSub1));
    }

    @Test
    public void renameTest() throws Exception {
        Path src = new Path("/bar");
        Path dst = new Path("/bard");

        byte[] data = new byte[]{1, 2, 4, 5};
        createWithContent(src, data);
        assertTrue(fileSystem.exists(src));
        assertTrue(!fileSystem.exists(dst));
        fileSystem.rename(src, dst);
        assertTrue(!fileSystem.exists(src));
        assertTrue(fileSystem.exists(dst));
        assertPathContent(dst, data);
    }

    @Test
    public void testInputStream() throws Exception {
        byte[] contents = new byte[]{-1, -2, -114, -65};
        Path p = new Path("/panda");
        createWithContent(p, contents);
        DataInputStream dis = new DataInputStream(fileSystem.open(p));
        int foo = dis.readInt();
        assertEquals(-94529, foo);
    }

    @Test
    public void appendTest() throws Exception {
        Path f = new Path("/peanuts");
        byte[] data1 = new byte[]{1, 2, 3, 4};
        byte[] data2 = new byte[]{5, 6, 7, 8};
        byte[] catenation = new byte[data1.length + data2.length];
        System.arraycopy(data1, 0, catenation, 0, data1.length);
        System.arraycopy(data2, 0, catenation, data1.length, data2.length);

        createWithContent(f, data1);
        FSDataOutputStream fsdo = fileSystem.append(f);
        fsdo.write(data2);
        fsdo.close();

        assertPathContent(f, catenation);
    }

    @Test
    public void testBlockSpanIo() throws Exception {
        int objectSize = 4;
        MemoryAmService mas = new MemoryAmService();
        mas.createVolume("volume", new VolumeSettings(objectSize, VolumeType.OBJECT, 4, 0, MediaPolicy.HDD_ONLY));
        FileSystem fs = new FdsFileSystem(mas, "fds://volume/", objectSize);

        Path f = new Path("/mr.meatloaf");
        byte[] chunk = new byte[]{1, 2, 3, 4, 5};
        FSDataOutputStream fsdo = fs.create(f);
        int chunks = 20 * (1 + (objectSize / chunk.length));
        for (int i = 0; i < chunks; i++)
            fsdo.write(chunk);
        fsdo.close();

        FSDataInputStream fsdi = fs.open(f);
        byte[] readChunk = new byte[chunk.length];
        for (int i = 0; i < chunks; i++) {
            fsdi.readFully(readChunk);
            assertArrayEquals(chunk, readChunk);
        }
        assertEquals(-1, fsdi.read());
    }

    @Test
    public void testWorkingDirectory() throws Exception {
        Path p1 = new Path("/foo");
        fileSystem.mkdirs(p1);

        fileSystem.setWorkingDirectory(p1);

        Path p2 = new Path("bar");
        fileSystem.mkdirs(p2);

        assertTrue(fileSystem.exists(new Path("/foo/bar")));
    }

    @Test
    public void testOpenDirectoryAsFile() throws Exception {
        Path p1 = new Path("/foo");
        fileSystem.mkdirs(p1);
        try {
            FSDataInputStream fsdi = fileSystem.open(p1);
            int readResult = fsdi.read();
            fsdi.close();
            fail("should not be able to call open() on a directory");
        } catch (IOException ex) {
            // expect this to throw an exception
        }
    }

    @Test
    public void testOpenNonexistentFile() throws Exception {
        try {
            Path f = new Path("/bar.txt");
            fileSystem.open(f);
            fail("this open should not succeed");
        } catch (FileNotFoundException ex) {
            // do nothing
        }
    }

    @Test
    public void deleteNonExistentFile() throws Exception {
        Path f = new Path("/bar.txt");
        boolean result = fileSystem.delete(f, false);
        assertFalse(result);
    }

    @Test
    public void testReadLong() throws Exception {
        Path path = new Path("/" + UUID.randomUUID().toString());
        FSDataOutputStream out = fileSystem.create(path, true);
        DataOutputStream dos = new DataOutputStream(out);
        out.writeLong(42);
        out.writeLong(43);
        out.writeLong(44);
        dos.close();
        IntStream.range(0, 100)
                .parallel()
                .forEach(i -> doSomeReads(path));
        ;
    }

    private void doSomeReads(Path path) {
        try {
            FSDataInputStream in = fileSystem.open(path);
            DataInputStream dis = new DataInputStream(in);
            assertEquals(42, dis.readLong());
            assertEquals(43, dis.readLong());
            assertEquals(44, dis.readLong());
            dis.close();
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    @Before
    public void setUpIntegration() throws Exception {
        XdiClientFactory xdiCf = new XdiClientFactory();
        ConfigurationService.Iface cs = xdiCf.remoteOmService("localhost", 9090);
        AmService.Iface am = xdiCf.remoteAmService("localhost", 9988);

        String tenantName = "hdfs-tenant-" + UUID.randomUUID().toString();
        String userName = "hdfs-user-" + UUID.randomUUID().toString();
        String volumeName = "hdfs-volume-" + UUID.randomUUID().toString();

        long tenantId = cs.createTenant(tenantName);
        long userId = cs.createUser(userName, "x", "x", false);
        cs.assignUserToTenant(userId, tenantId);

        cs.createVolume(FdsFileSystem.DOMAIN, volumeName, new VolumeSettings(OBJECT_SIZE, VolumeType.OBJECT, 0, 0, MediaPolicy.HDD_ONLY), userId);
        fileSystem = new FdsFileSystem(am, "fds://" + volumeName + "/", OBJECT_SIZE);
    }

    @After
    public void tearDown() throws Exception {

    }

    private void touch(Path f) throws IOException {
        fileSystem.create(f).close();
    }

    private void createWithContent(Path f, byte[] data) throws IOException {
        FSDataOutputStream fso = fileSystem.create(f);
        fso.write(data);
        fso.close();
    }

    private void assertPathContent(Path f, byte[] data) throws IOException {
        FSDataInputStream fsi = fileSystem.open(f);
        byte[] readBuf = new byte[data.length];
        fsi.readFully(readBuf);
        assertEquals(-1, fsi.read());
        assertArrayEquals(data, readBuf);
    }
}

package com.formationds.smoketest;

import com.formationds.apis.ConfigurationService;
import com.formationds.apis.MediaPolicy;
import com.formationds.apis.VolumeSettings;
import com.formationds.apis.VolumeType;
import com.formationds.commons.Fds;
import com.formationds.hadoop.FdsFileSystem;
import com.formationds.hadoop.OwnerGroupInfo;
import com.formationds.xdi.XdiClientFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.FSDataInputStream;
import org.apache.hadoop.fs.FSDataOutputStream;
import org.apache.hadoop.fs.FileStatus;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.fs.permission.FsPermission;
import org.apache.hadoop.util.Progressable;
import org.junit.After;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Test;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.net.URI;
import java.util.Arrays;
import java.util.UUID;
import java.util.stream.IntStream;

import static org.junit.Assert.*;


@Ignore
public class HdfsSmokeTest{
    private final int OBJECT_SIZE = 1024 * 1024 * 2;
    private FdsFileSystem fileSystem;

    @Test(expected = IOException.class)
    public void testMakeDirectoryAttempt() throws Exception {
        Path p = new Path("/foo");
        touch(p);
        fileSystem.mkdirs(p);
    }

    // @Test
    public void testHdfsPerformance() throws Exception {
        Path path = new Path("/foo.bin");
        FSDataOutputStream outputStream = fileSystem.create(path);
        byte[] buf = new byte[OBJECT_SIZE];
        long then = System.currentTimeMillis();
        int objectCount = 200;
        for (int i = 0; i < objectCount; i++) {
            outputStream.write(buf);
        }
        outputStream.flush();
        outputStream.close();
        long elapsed = System.currentTimeMillis() - then;
        double throughputInMB = 1000 * (objectCount * 2) / (double) elapsed;
        double fourKBlocksInOneMB = OBJECT_SIZE / 4096d;

        System.out.println("Wrote " + objectCount + " objects in " + elapsed + "ms ");
        System.out.println("Write throughput: " + throughputInMB + " mB/s ");
        System.out.println("Write equivalent: " + throughputInMB * fourKBlocksInOneMB + " IOPs");

        FSDataInputStream input = fileSystem.open(path);
        then = System.currentTimeMillis();
        int readCount = 0;
        for (int i = input.read(buf); i != -1; i = input.read(buf)) {
            readCount++;
        }
        elapsed = System.currentTimeMillis() - then;
        throughputInMB = 1000 * (readCount * 2) / (double) elapsed;

        System.out.println("Read " + readCount + " objects in " + elapsed + "ms ");
        System.out.println("Read throughput: " + throughputInMB + " mB/s ");
        System.out.println("Read equivalent: " + throughputInMB * fourKBlocksInOneMB + " IOPs");

    }

    @Test
    public void testDeleteInexistentFile() throws Exception {
        Path f = new Path("/foo");
        try{
            fileSystem.delete(f, false);
            fail("File does not exists to delete");
        } catch (FileNotFoundException e) {

        }
    }

    @Test
    public void testPermissions() throws Exception {
        Path f = new Path("/foo");
        touch(f);
        OwnerGroupInfo currentUser = OwnerGroupInfo.current();
        assertEquals(currentUser.getOwner(), fileSystem.getFileStatus(f).getOwner());
        assertEquals(currentUser.getGroup(), fileSystem.getFileStatus(f).getGroup());

        Path d = new Path("/bar");

        fileSystem.mkdirs(d);
        assertEquals(currentUser.getOwner(), fileSystem.getFileStatus(d).getOwner());
        assertEquals(currentUser.getGroup(), fileSystem.getFileStatus(d).getGroup());
    }

    @Test
    public void testListFiles() throws Exception {
        touch(new Path("/a"));

        Path dir = new Path("/foo");
        fileSystem.mkdirs(dir);

        touch(new Path("/z"));

        String[] fileNames = new String[]{"aaa", "aab", "aac"};

        Arrays.stream(fileNames)
                .forEach(f -> createWithContent(new Path("/foo/" + f), f.getBytes()));

        FileStatus[] statuses = fileSystem.listStatus(dir);
        for (int i = 0; i < statuses.length; i++) {
            FileStatus status = statuses[i];
            assertEquals(3, status.getLen());
            assertEquals(fileNames[i], status.getPath().getName());
        }

        FileStatus[] rootObjects = fileSystem.listStatus(new Path("/"));
        assertEquals(3, rootObjects.length);
        assertEquals("a", rootObjects[0].getPath().getName());
        assertEquals("foo", rootObjects[1].getPath().getName());
        assertEquals("z", rootObjects[2].getPath().getName());

        Path notDir = new Path("/notDirectory");
        touch (notDir);
        assertTrue(fileSystem.exists(notDir));
        FileStatus[] status = fileSystem.listStatus(notDir);
        assertEquals(1, status.length);
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
        byte[] largeContents = new byte[size];
        largeContents[size - 1] = 42;
        createWithContent(p, largeContents);
        FileStatus st = fileSystem.getFileStatus(p);
        assertEquals(size, (int) st.getLen());
        byte[] result = org.apache.commons.io.IOUtils.toByteArray(fileSystem.open(p));
        assertArrayEquals(largeContents, result);

        int smallSize = 1024;
        byte[] smallContents = new byte[smallSize];
        smallContents[smallSize - 1] = 43;
        createWithContent(p, smallContents);
        FileStatus status = fileSystem.getFileStatus(p);
        assertEquals(smallSize, (int) status.getLen());
        byte[] resultNew = org.apache.commons.io.IOUtils.toByteArray(fileSystem.open(p));
        assertArrayEquals(smallContents, resultNew);
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
    public void testRenameRecursive() throws Exception {
        fileSystem.mkdirs(new Path("/foo/bar"), FsPermission.getDefault());
        byte[] data = new byte[]{1, 2, 4, 5};
        createWithContent(new Path("/foo/bar/hello"), data);
        assertTrue(fileSystem.exists(new Path("/foo/bar/hello")));
        fileSystem.rename(new Path("/foo"), new Path("/panda"));
        assertTrue(fileSystem.exists(new Path("/panda/bar/hello")));
        assertFalse(fileSystem.exists(new Path("/foo/bar/hello")));
    }

    @Test
    public void testInputStream() throws Exception {
        byte[] contents = new byte[]{-1, -2, -114, -65};
        Path p = new Path("/panda");
        createWithContent(p, contents);
        DataInputStream dis = new DataInputStream(fileSystem.open(p));
        int foo = dis.readInt();
        assertEquals(-94529, foo);
        dis.close();

        dis = new DataInputStream(fileSystem.open(p));
        for (int i = 0; i < contents.length; i++) {
            int val = dis.readByte();
            assertEquals(contents[i], val);
        }
    }

    @Test
    public void testCreateNonRecursive() throws Exception {
        byte[] contents = new byte[]{-1, -2, -114, -65};
        Path p = new Path("/foo/bar");
        Progressable progress = () -> {
        };

        try {
            fileSystem.createNonRecursive(p, FsPermission.getDefault(), true, 1024, (short) 1, 1024, progress);
            fail("Should have gotten an IOException!");
        } catch (IOException e) {
        }

        fileSystem.mkdirs(new Path("/foo"));
        FSDataOutputStream out = fileSystem.createNonRecursive(p, FsPermission.getDefault(), true, 1024, (short) 1, 1024, progress);
        out.write(contents);
        out.flush();
        out.close();
        assertPathContent(p, contents);
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
    }

    private synchronized void doSomeReads(Path path) {
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
        String host = Fds.getFdsHost();
        ConfigurationService.Iface cs = xdiCf.remoteOmService(host, 9090);

        String tenantName = "hdfs-tenant-" + UUID.randomUUID().toString();
        String userName = "hdfs-user-" + UUID.randomUUID().toString();
        String volumeName = "hdfs-volume-" + UUID.randomUUID().toString();

        long tenantId = cs.createTenant(tenantName);
        long userId = cs.createUser(userName, "x", "x", false);
        cs.assignUserToTenant(userId, tenantId);

        cs.createVolume(FdsFileSystem.DOMAIN, volumeName, new VolumeSettings(OBJECT_SIZE, VolumeType.OBJECT, 0, 0, MediaPolicy.HDD_ONLY), userId);
        Configuration hadoopConf = new Configuration();
        hadoopConf.set("fds.am.endpoint", Fds.getFdsHost());
        hadoopConf.set("fds.cs.endpoint", Fds.getFdsHost());
        URI uri = new URI("fds://" + volumeName + "/");
        fileSystem = new FdsFileSystem(uri, hadoopConf);
    }

    @After
    public void tearDown() throws Exception {

    }

    private void touch(Path f) throws IOException {
        fileSystem.create(f).close();
    }

    private void createWithContent(Path f, byte[] data) {
        try {
            FSDataOutputStream fso = fileSystem.create(f);
            fso.write(data);
            fso.close();
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    private void assertPathContent(Path f, byte[] data) throws IOException {
        FSDataInputStream fsi = fileSystem.open(f);
        byte[] readBuf = new byte[data.length];
        fsi.readFully(readBuf);
        assertEquals(-1, fsi.read());
        assertArrayEquals(data, readBuf);
    }
}

package com.formationds.hadoop;

import org.apache.hadoop.fs.FSDataOutputStream;
import org.apache.hadoop.fs.Path;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.io.OutputStreamWriter;
import java.io.PrintWriter;

import static org.junit.Assert.assertEquals;

public class FdsFileSystemTest {

    private FdsFileSystem fileSystem;

    @Test
    public void testListFiles() throws Exception {
        Path f = new Path("fds://volume/foo/bar.txt");
        FSDataOutputStream stream = fileSystem.create(f);
        PrintWriter printWriter = new PrintWriter(new OutputStreamWriter(stream));
        printWriter.append("foo\n");
        printWriter.close();
        assertEquals(4, fileSystem.getFileStatus(f).getLen());
        assertEquals(1, fileSystem.listStatus(new Path("fds://volume/foo")).length);
    }

    @Before
    public void setUp() throws Exception {
        fileSystem = new FdsFileSystem();
        fileSystem.mkdirs(new Path("fds://volume/foo/"));
    }

    @After
    public void tearDown() throws Exception {
        fileSystem.delete(new Path("fds://volume/foo/"), true);
    }
}
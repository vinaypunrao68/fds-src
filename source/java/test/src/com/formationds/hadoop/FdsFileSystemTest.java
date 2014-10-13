package com.formationds.hadoop;

import org.apache.hadoop.fs.FSDataOutputStream;
import org.apache.hadoop.fs.Path;
import org.junit.Test;

import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.net.URI;

public class FdsFileSystemTest {
    // @Test
    public void testInit() throws Exception {
        FdsFileSystem fileSystem = new FdsFileSystem();
        FSDataOutputStream stream = fileSystem.create(new Path("fds:///hello.txt"));
        PrintWriter printWriter = new PrintWriter(new OutputStreamWriter(stream));
        printWriter.append("foo\n");
        printWriter.append("bar\n");
        printWriter.close();
    }
}
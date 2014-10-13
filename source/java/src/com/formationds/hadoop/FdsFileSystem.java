package com.formationds.hadoop;

import org.apache.commons.io.FileUtils;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.*;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.permission.FsPermission;
import org.apache.hadoop.util.Progressable;

import java.io.*;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.Arrays;

/**
 * Copyright (c) 2014 Formation Data Systems, Inc.
 */
public class FdsFileSystem extends FileSystem {
    private final static String PREFIX = "/tmp";
    private Path workingDirectory;
    private URI uri;

    public FdsFileSystem() throws URISyntaxException, IOException {
        uri = new URI("fds://volume/");
        workingDirectory = new Path(uri);
        setConf(new Configuration());
        FileUtils.forceMkdir(new File(PREFIX));
    }

    @Override
    public void initialize(URI uri, Configuration conf) throws IOException {
        setConf(conf);
        this.uri = URI.create(getScheme() + "://" + uri.getAuthority());
        workingDirectory = new Path(uri);
    }

    @Override
    public String getScheme() {
        return "fds";
    }

    @Override
    public URI getUri() {
        return uri;
    }

    @Override
    public FSDataInputStream open(Path f, int bufferSize) throws IOException {
        return new FSDataInputStream(new FileInputStream(pathToFile(f)));
    }

    @Override
    public FSDataOutputStream create(Path f, FsPermission permission, boolean overwrite, int bufferSize, short replication, long blockSize, Progressable progress) throws IOException {
        return new FSDataOutputStream(new FileOutputStream(pathToFile(f)));
    }

    @Override
    public FSDataOutputStream append(Path f, int bufferSize, Progressable progress) throws IOException {
        return new FSDataOutputStream(new FileOutputStream(pathToFile(f)));
    }

    @Override
    public boolean rename(Path src, Path dst) throws IOException {
        FileUtils.moveFile(pathToFile(src), pathToFile(dst));
        return true;
    }

    @Override
    public boolean delete(Path f, boolean recursive) throws IOException {
        FileUtils.deleteQuietly(pathToFile(f));
        return true;
    }

    @Override
    public FileStatus[] listStatus(Path f) throws FileNotFoundException, IOException {
        File file = pathToFile(f);
        if (file.isDirectory()) {
            return Arrays.stream(file.listFiles())
                    .map(e -> new FileStatus(e.length(), e.isDirectory(), 1, 4096, e.lastModified(), new Path(f, e.getName())))
                    .toArray(i -> new FileStatus[i]);
        } else {
            return new FileStatus[] {new FileStatus(file.length(), file.isDirectory(), 1, 4096, file.lastModified(), new Path(f, file.getName()))};
        }
    }

    @Override
    public Path getWorkingDirectory() {
        return workingDirectory;
    }

    @Override
    public void setWorkingDirectory(Path new_dir) {
        workingDirectory = new_dir;
    }

    @Override
    public boolean mkdirs(Path f, FsPermission permission) throws IOException {
        File file = pathToFile(f);
        FileUtils.forceMkdir(file);
        return true;
    }

    @Override
    public BlockLocation[] getFileBlockLocations(Path p, long start, long len) throws IOException {
        return super.getFileBlockLocations(p, start, len);
    }

    @Override
    public FileStatus getFileStatus(Path f) throws IOException {
        File file = pathToFile(f);
        if (!file.exists()) {
            throw new FileNotFoundException();
        }
        return new FileStatus(file.length(), file.isDirectory(), 1, 4096, file.lastModified(), f);
    }


    /** Convert a path to a File. */
    public File pathToFile(Path path) {
        checkPath(path);
        if (!path.isAbsolute()) {
            path = new Path(getWorkingDirectory(), path);
        }

        URI uri = path.toUri();
        String s = uri.toString().replaceAll("fds://volume/", PREFIX + "/");
        return new File(s);
    }
}

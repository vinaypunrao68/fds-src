package com.formationds.hadoop;

import com.formationds.apis.*;
import com.formationds.protocol.BlobListOrder;
import com.formationds.util.HostAndPort;
import com.formationds.util.blob.Mode;
import com.formationds.xdi.XdiClientFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.*;
import org.apache.hadoop.fs.permission.FsPermission;
import org.apache.hadoop.security.UserGroupInformation;
import org.apache.hadoop.util.Progressable;
import org.apache.thrift.TException;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.net.URI;
import java.net.URISyntaxException;
import java.nio.ByteBuffer;
import java.util.*;

/**
 * Copyright (c) 2014 Formation Data Systems, Inc.
 */

// NOTE: this class has serious limitations
//
// - the implementation of directories is basically as blobs with a particular chunk of metadata set -
//   there are potentials for races everywhere since we cannot transactionally check if a blob exists
//   and is a directory before creating a file - we could assume that we don't need to make blobs for
//   directories, but then we will have a problem when mkdir() does not actually have an noticable effect
//   on the filesystem
//
// - platform does not currently support indexing of blob names, any corresponding operations are going to be
//   unreasonably slow as the size of the filesystem increases
//
// - locality is not implemented at the platform level - once locality is implemented we may need to revise this
//   class to inform it how to get data locally
//
// - platform support does not exist for rename() - it does a copy and then a delete instead
//
public class FdsFileSystem extends FileSystem {
    public static final String DOMAIN = "HDFS";
    public static final String DIRECTORY_SPECIFIER_KEY = "directory";
    public static final String LAST_MODIFIED_KEY = "last-modified";
    public static final String CURRENT_OFFSET = "current-offset";

    private AmService.Iface am;
    private Path workingDirectory;
    private URI uri;
    private int blockSize;

    public FdsFileSystem() {

    }

    public FdsFileSystem(URI uri, Configuration conf) throws IOException {
        initialize(uri, conf);
    }

    public FdsFileSystem(AmService.Iface am, String uri, int blockSize) throws URISyntaxException, IOException {
        this.am = am;
        this.blockSize = blockSize;
        this.uri = new URI(uri);
        this.workingDirectory = new Path(uri);
        setConf(new Configuration());
        createRootIfNeeded();
    }

    @Override
    public void initialize(URI uri, Configuration conf) throws IOException {
        setConf(conf);
        String am = conf.get("fds.am.endpoint");
        String cs = conf.get("fds.cs.endpoint");

        XdiClientFactory cf = new XdiClientFactory();
        AmService.Iface amClient = null;

        this.uri = URI.create(getScheme() + "://" + uri.getAuthority());
        workingDirectory = new Path(uri);

        if (am != null) {
            HostAndPort amConnectionData = HostAndPort.parseWithDefaultPort(am, 9988);
            amClient = cf.remoteAmService(amConnectionData.getHost(), amConnectionData.getPort());
        } else if (this.am != null) {
            amClient = this.am;
        } else {
            throw new IOException("required configuration key 'fds.am.endpoint' with format HOST:PORT not found in configuration");
        }
        this.am = amClient;

        if (cs != null) {
            ConfigurationService.Iface csClient = null;
            HostAndPort csConnectionData = HostAndPort.parseWithDefaultPort(cs, 9090);
            csClient = cf.remoteOmService(csConnectionData.getHost(), csConnectionData.getPort());
            try {
                VolumeDescriptor status = csClient.statVolume(DOMAIN, getVolume());
                if (status == null)
                    throw new FileNotFoundException();
                blockSize = status.getPolicy().getMaxObjectSizeInBytes();
            } catch (ApiException ex) {
                if (ex.getErrorCode() == ErrorCode.MISSING_RESOURCE)
                    throw new FileNotFoundException();
                throw new IOException(ex);
            } catch (Exception e) {
                throw new IOException(e);
            }
        } else if (blockSize == 0) {
            throw new IOException("required configuration key 'fds.cs.endpoint' with format HOST:PORT not found in configuration");
        }

        createRootIfNeeded();
    }

    private void createRootIfNeeded() throws IOException {
        Path root = new Path(uri + "/");
        try {
            am.statBlob(DOMAIN, getVolume(), root.toString());
        } catch (ApiException e) {
            if (e.getErrorCode().equals(ErrorCode.MISSING_RESOURCE)) {
                mkDirBlob(root);
            }
        } catch (Exception e) {
            throw new IOException(e);
        }
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
    public FSDataInputStream open(Path path, int bufferSize) throws IOException {
        Path absolutePath = getAbsolutePath(path);
        checkIsValidReadTarget(path);
        return new FSDataInputStream(new FdsInputStream(am, DOMAIN, getVolume(), absolutePath.toString(), blockSize));
    }

    @Override
    public FSDataOutputStream create(Path path, FsPermission permission, boolean overwrite, int bufferSize, short replication, long blockSize, Progressable progress) throws IOException {
        Path absolutePath = getAbsolutePath(path);
        if (!exists(absolutePath.getParent())) {
            mkdirs(absolutePath.getParent());
        }
        checkIsValidWriteTarget(absolutePath);
        return new FSDataOutputStream(FdsOutputStream.openNew(am, DOMAIN, getVolume(), absolutePath.toString(), getBlockSize()));
    }

    @Override
    public FSDataOutputStream append(Path path, int bufferSize, Progressable progress) throws IOException {
        Path absolutePath = getAbsolutePath(path);
        checkIsValidWriteTarget(absolutePath);
        return new FSDataOutputStream(FdsOutputStream.openForAppend(am, DOMAIN, getVolume(), absolutePath.toString(), getBlockSize()));
    }


    // TODO: we need an FDS implementation
    @Override
    public boolean rename(Path srcPath, Path dstPath) throws IOException {
        Path srcAbsolutePath = getAbsolutePath(srcPath);
        Path dstAbsolutePath = getAbsolutePath(dstPath);

        if (isDirectory(srcAbsolutePath)) {
            FileStatus[] file = listStatus(srcAbsolutePath);
            for (FileStatus fileStatus : file) {
                if (fileStatus.isDirectory()) {
                    rename(fileStatus.getPath(), new Path(dstAbsolutePath, fileStatus.getPath().getName()));
                } else {
                    renameFile(fileStatus.getPath(), new Path(dstAbsolutePath, fileStatus.getPath().getName()));
                }
            }
            return true;
        } else {
            return renameFile(srcPath, dstPath);
        }
    }

    private boolean renameFile(Path srcAbsolutePath, Path dstAbsolutePath) throws IOException {

        checkIsValidReadTarget(srcAbsolutePath);

        if (!exists(dstAbsolutePath.getParent())) {
            mkdirs(dstAbsolutePath.getParent());
        }

        checkIsValidWriteTarget(dstAbsolutePath);
        FSDataInputStream in = open(srcAbsolutePath);
        FSDataOutputStream out = create(dstAbsolutePath);

        byte[] data = new byte[8192];
        int readLength = 0;
        while ((readLength = in.read(data)) != -1) {
            out.write(data, 0, readLength);
        }
        out.close();
        delete(srcAbsolutePath, false);
        return true;
    }

    @Override
    public boolean delete(Path path, boolean recursive) throws IOException {
        Path absolutePath = getAbsolutePath(path);
        try {
            List<Path> subPaths = getAllSubPaths(absolutePath, true);
            if (subPaths.isEmpty())
                return false;

            if (recursive) {
                for (Path subPath : subPaths) {
                    am.deleteBlob(DOMAIN, getVolume(), subPath.toString());
                }
            } else if (subPaths.size() != 1) {
                return false;
            } else {
                am.deleteBlob(DOMAIN, getVolume(), absolutePath.toString());
            }
        } catch (Exception ex) {
            throw new IOException(ex);
        }
        return true;
    }

    @Override
    public FileStatus[] listStatus(Path path) throws FileNotFoundException, IOException {
        Path absolutePath = getAbsolutePath(path);
        List<Path> contents = getPathContents(absolutePath);
        ArrayList<FileStatus> status = new ArrayList<>(contents.size());
        for (Path elt : contents) {
            try {
                status.add(getFileStatus(elt));
            } catch (FileNotFoundException ex) {
                // do nothing
            }
        }
        return status.toArray(new FileStatus[status.size()]);
    }

    @Override
    public Path getWorkingDirectory() {
        return workingDirectory;
    }

    @Override
    public void setWorkingDirectory(Path newDir) {
        workingDirectory = getAbsolutePath(newDir);
    }

    @Override
    public boolean mkdirs(Path path, FsPermission permission) throws IOException {
        Path absolutePath = getAbsolutePath(path);
        if (!absolutePath.getParent().isRoot()) {
            mkdirs(absolutePath.getParent(), permission);
        }
        mkDirBlob(absolutePath);
        return true;
    }

    @Override
    public BlockLocation[] getFileBlockLocations(Path p, long start, long len) throws IOException {
        // TODO: we can't implement this without exposing locality at the platform level
        return super.getFileBlockLocations(p, start, len);
    }

    @Override
    public FileStatus getFileStatus(Path path) throws IOException {
        Path absolutePath = getAbsolutePath(path);
        UserGroupInformation ugi = UserGroupInformation.getCurrentUser();
        String currentUser = ugi.getShortUserName();
        String groupName = ugi.getShortUserName(); //ugi.getPrimaryGroupName();

        try {
            BlobDescriptor bd = am.statBlob(DOMAIN, getVolume(), absolutePath.toString());
            boolean isDirectory = isDirectory(bd);
            FsPermission permission = isDirectory ? FsPermission.getDirDefault() : FsPermission.getFileDefault();
            long byteCount = getByteCount(bd);
            return new FileStatus(byteCount, isDirectory, 1, 2 * 1024 * 1024, getMtime(bd),
                    getMtime(bd), permission, currentUser, groupName, absolutePath);
        } catch (ApiException ex) {
            if (ex.getErrorCode() == ErrorCode.MISSING_RESOURCE) {
                throw new FileNotFoundException(path.toString());
            } else {
                throw new IOException(ex);
            }
        } catch (Exception ex) {
            throw new IOException(ex);
        }
    }

    public static long getByteCount(BlobDescriptor bd) {
        return bd.getMetadata().containsKey(CURRENT_OFFSET) ? Long.parseLong(bd.getMetadata().get(CURRENT_OFFSET)) : bd.getByteCount();
    }

    private void checkIsValidWriteTarget(Path path) throws IOException {
        checkParent(path);
        if (exists(path) && isDirectory(path))
            throw new IOException(path.toString() + " is a directory");
    }

    private void checkIsValidReadTarget(Path path) throws IOException {
        checkParent(path);
        if (!exists(path))
            throw new FileNotFoundException(path.toString() + " does not exist");
        if (isDirectory(path))
            throw new IOException(path.toString() + " is a directory");
    }

    private void checkParent(Path path) throws IOException {
        // TODO: there are serious problems with this approach, there is nothing preventing someone from deleting the directory after the check, for example
        if (path.getParent().isRoot())
            return;

        if (!exists(path.getParent()))
            throw new FileNotFoundException("parent directory does not exist");

        if (!isDirectory(path.getParent())) {
            throw new IOException("parent is not a directory");
        }
    }

    private String getVolume() {
        return uri.getAuthority();
    }

    private void mkDirBlob(Path path) throws IOException {
        String targetPath = path.toString();
        try {
            Map<String, String> map = new HashMap<>();
            map.put(DIRECTORY_SPECIFIER_KEY, "true");
            am.updateBlobOnce(DOMAIN, getVolume(), targetPath, Mode.TRUNCATE.getValue(), ByteBuffer.allocate(0), 0, new ObjectOffset(0), map);
        } catch (Exception ex) {
            throw new IOException(ex);
        }
    }

    private boolean isDirectory(BlobDescriptor bd) {
        return bd.metadata.containsKey(DIRECTORY_SPECIFIER_KEY);
    }

    private long getMtime(BlobDescriptor bd) {
        if (!bd.metadata.containsKey(LAST_MODIFIED_KEY)) {
            return Calendar.getInstance().getTimeInMillis();
        } else {
            String data = bd.metadata.get(LAST_MODIFIED_KEY);
            return Long.parseLong(data);
        }
    }

    private int getBlockSize() throws IOException {
        return blockSize;
    }

    private List<BlobDescriptor> getAllBlobDescriptors() throws ApiException, TException {
        return am.volumeContents(DOMAIN, getVolume(), Integer.MAX_VALUE, 0, "", BlobListOrder.UNSPECIFIED, false);
    }

    private List<Path> getAllSubPaths(Path path, boolean includeSelf) throws IOException {
        try {
            List<Path> paths = new ArrayList<>();
            List<BlobDescriptor> descriptors = am.volumeContents(DOMAIN, getVolume(), Integer.MAX_VALUE, 0, "", BlobListOrder.UNSPECIFIED, false);
            for (BlobDescriptor bd : descriptors) {
                Path blobPath = new Path(bd.getName());
                if (isParent(path, blobPath))
                    paths.add(blobPath);
                else if (includeSelf && path.equals(blobPath))
                    paths.add(blobPath);
            }
            return paths;
        } catch (Exception ex) {
            throw new IOException(ex);
        }
    }

    private boolean isParent(Path path, Path other) {
        Path p = other.getParent();
        while (p != null) {
            if (path.equals(p))
                return true;
            p = p.getParent();
        }
        return false;
    }

    // TODO: fix when we get indexed paths
    private List<Path> getPathContents(Path path) throws IOException {
        try {
            List<Path> paths = new ArrayList<>();
            List<BlobDescriptor> descriptors = am.volumeContents(DOMAIN, getVolume(), Integer.MAX_VALUE, 0, "", BlobListOrder.UNSPECIFIED, false);
            for (BlobDescriptor bd : descriptors) {
                Path name = new Path(bd.getName());
                if (name.getParent() != null && name.getParent().equals(path)) {
                    paths.add(name);
                }
            }
            return paths;
        } catch (Exception ex) {
            throw new IOException(ex);
        }
    }

    public Path getAbsolutePath(Path path) {
        path = new Path(path.toString().replaceAll("#.+$", ""));
        if (!path.toString().startsWith("fds://")) {
            Path p = new Path(workingDirectory.toString(), path.toString());
            return p;
        }
        return path;
    }

    public static class AFS extends DelegateToFileSystem {
        public AFS(URI uri, Configuration configuration) throws IOException, URISyntaxException {
            super(uri, new FdsFileSystem(uri, configuration), configuration, "fds", false);
        }

        @Override
        public int getUriDefaultPort() {
            return -1;
        }


    }
}

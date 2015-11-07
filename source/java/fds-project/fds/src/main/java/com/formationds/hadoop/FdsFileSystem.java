package com.formationds.hadoop;

import com.formationds.apis.ConfigurationService;
import com.formationds.apis.ObjectOffset;
import com.formationds.apis.VolumeDescriptor;
import com.formationds.protocol.*;
import com.formationds.util.HostAndPort;
import com.formationds.util.ServerPortFinder;
import com.formationds.commons.util.SupplierWithExceptions;
import com.formationds.util.blob.Mode;
import com.formationds.xdi.AsyncAm;
import com.formationds.xdi.RealAsyncAm;
import com.formationds.xdi.XdiClientFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.*;
import org.apache.hadoop.fs.permission.FsPermission;
import org.apache.hadoop.util.Progressable;
import org.apache.log4j.Logger;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.net.URI;
import java.net.URISyntaxException;
import java.nio.ByteBuffer;
import java.util.*;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;

/**
 * Copyright (c) 2014 Formation Data Systems, Inc.
 */

// NOTE: this class has limitations
//
// - getFileStatus() results are not cached, so we do many statBlob()s as a result
//
// - locality is not implemented at the platform level - once locality is implemented we may need to revise this
//   class to inform it how to get data locally
//
// - the implementation of directories is basically as blobs with a particular chunk of metadata set -
//   there are potentials for races everywhere since we cannot transactionally check if a blob exists
//   and is a directory before creating a file - we could assume that we don't need to make blobs for
//   directories, but then we will have a problem when mkdir() does not actually have an noticable effect
//   on the filesystem
//
// - platform support does not exist for rename() - it does a copy and then a delete instead
//
public class FdsFileSystem extends FileSystem {
    private static final Logger LOG = Logger.getLogger(FdsFileSystem.class);
    public static final String DOMAIN = "HDFS";
    public static final String DIRECTORY_SPECIFIER_KEY = "directory";
    public static final String LAST_MODIFIED_KEY = "last-modified";
    public static final String CREATED_BY_USER = "created-by-user";
    public static final String CREATED_BY_GROUP = "created-by-group";

    private Path workingDirectory;
    private URI uri;
    private int blockSize;
    private AsyncAm asyncAm;

    public FdsFileSystem() {
    }

    public FdsFileSystem(URI uri, Configuration conf) throws IOException {
        this();
        initialize(uri, conf);
    }

    @Override
    public void initialize(URI uri, Configuration conf) throws IOException {
        setConf(conf);
        String am = conf.get("fds.am.endpoint");
        String cs = conf.get("fds.cs.endpoint");

        int amResponsePort = new ServerPortFinder().findPort("HDFS async AM response port", 10000);
        HostAndPort amConnectionData = HostAndPort.parseWithDefaultPort(am, 8899);
        XdiClientFactory cf = new XdiClientFactory();

        try {
            asyncAm = new RealAsyncAm(
                    amConnectionData.getHost(),
                    amConnectionData.getPort(),
                    amResponsePort,
                    10,
                    TimeUnit.SECONDS);

            asyncAm.start();
        } catch (Exception e) {
            LOG.error("Error starting async AM", e);
            throw new IOException(e);
        }

        this.uri = URI.create(getScheme() + "://" + uri.getAuthority());
        workingDirectory = new Path(uri);

        ConfigurationService.Iface csClient = null;
        HostAndPort csConnectionData = HostAndPort.parseWithDefaultPort(cs, 9090);
        csClient = cf.remoteOmService(csConnectionData.getHost(), csConnectionData.getPort());
        try {
            VolumeDescriptor status = csClient.statVolume(DOMAIN, getVolume());
            if (status == null)
                throw new FileNotFoundException();
            blockSize = status.getPolicy().getMaxObjectSizeInBytes();
        } catch (ApiException ex) {
            LOG.error("Error getting volume info", ex);
            if (ex.getErrorCode() == ErrorCode.MISSING_RESOURCE)
                throw new FileNotFoundException();
            throw new IOException(ex);
        } catch (Exception e) {
            LOG.error("Error getting volume info", e);
            throw new IOException(e);
        }

        createRootIfNeeded();
    }
    private void createRootIfNeeded() throws IOException {
        Path root = new Path(uri + "/");
        try {
            unwindExceptions(() -> asyncAm.statBlob(DOMAIN, getVolume(), root.toString()).get());
        } catch (ApiException e) {
            if (e.getErrorCode().equals(ErrorCode.MISSING_RESOURCE)) {
                mkDirBlob(root);
            } else {
                throw new IOException(e);
            }
        } catch (Exception e) {
            throw new IOException(e);
        }
    }

    public static <T> T unwindExceptions(SupplierWithExceptions<T> supplier) throws ApiException, Exception {
        try {
            return supplier.supply();
        } catch (ExecutionException e) {
            if (e.getCause() instanceof ApiException) {
                throw (ApiException) e.getCause();
            } else {
                throw new Exception(e.getCause());
            }
        } catch (Exception e) {
            throw e;
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
        ensureReadingPossible(absolutePath);
        return new FSDataInputStream(new FdsInputStream(asyncAm, DOMAIN, getVolume(), absolutePath.toString(), blockSize));
    }

    private void ensureReadingPossible(Path absolutePath) throws IOException {
        FileStatus fileStatus = getFileStatus(absolutePath);
        if (fileStatus.isDirectory()) {
            throw new IOException("Path is a directory");
        }
    }

    @Override
    public FSDataOutputStream create(Path path, FsPermission permission, boolean overwrite, int bufferSize, short replication, long blockSize, Progressable progress) throws IOException {
        Path absolutePath = getAbsolutePath(path);
        if (!exists(absolutePath.getParent())) {
            mkdirs(absolutePath.getParent());
        }
        ensureWritingPossible(absolutePath);
        return new FSDataOutputStream(FdsOutputStream.openNew(asyncAm, DOMAIN, getVolume(), absolutePath.toString(), getBlockSize(), OwnerGroupInfo.current()));
    }

    @Override
    public FSDataOutputStream createNonRecursive(Path f, FsPermission permission,
                                                 EnumSet<CreateFlag> flags, int bufferSize, short replication, long blockSize,
                                                 Progressable progress) throws IOException {
        Path absolutePath = getAbsolutePath(f);
        if (!exists(absolutePath.getParent())) {
            throw new IOException("Parent does not exist!");
        }
        ensureWritingPossible(absolutePath);
        return new FSDataOutputStream(FdsOutputStream.openNew(asyncAm, DOMAIN, getVolume(), absolutePath.toString(), getBlockSize(), OwnerGroupInfo.current()));

    }

    private void ensureWritingPossible(Path absolutePath) throws IOException {
        FileStatus fileStatus = null;
        try {
            fileStatus = getFileStatus(absolutePath);
        } catch (FileNotFoundException e) {
            return;
        }

        if (fileStatus.isDirectory()) {
            throw new IOException("Path is a directory");
        }
    }


    @Override
    public FSDataOutputStream append(Path path, int bufferSize, Progressable progress) throws IOException {
        Path absolutePath = getAbsolutePath(path);
        ensureReadingPossible(absolutePath);
        return new FSDataOutputStream(FdsOutputStream.openForAppend(asyncAm, DOMAIN, getVolume(), absolutePath.toString(), getBlockSize()));
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
	Path originalPath = getAbsolutePath(srcAbsolutePath);
	Path newPath = getAbsolutePath(dstAbsolutePath);
        try {
	    asyncAm.renameBlob(DOMAIN, getVolume(), originalPath.toString(), newPath.toString()).get();
	} catch (Exception e) {
	    throw new IOException(e);
	}
        return true;
    }

    @Override
    public boolean delete(Path path, boolean recursive) throws IOException {
        Path absolutePath = getAbsolutePath(path);
        FileStatus status = null;
        try {
            status = getFileStatus(path);
        } catch (FileNotFoundException e) {
                  throw new FileNotFoundException("Trying to delete non existing object");
        }

        try {
            if (status.isDirectory()) {
                if (!recursive)
                    throw new IOException("HDFS: please use recursive delete for directories");
                List<BlobDescriptor> bds = listAllSubPaths(absolutePath, true, true);
                for (BlobDescriptor bd : bds) {
                    asyncAm.deleteBlob(DOMAIN, getVolume(), bd.getName()).get();
                }
            } else {
                asyncAm.deleteBlob(DOMAIN, getVolume(), absolutePath.toString()).get();
            }
            return true;
        } catch (Exception e) {
            throw new IOException(e);
        }
    }

    private List<BlobDescriptor> listAllSubPaths(Path path, boolean recursive, boolean descending) throws IOException {
        Path absolutePath = getAbsolutePath(path);
        FileStatus fileStatus = getFileStatus(path);

        if (!fileStatus.isDirectory()) {
            throw new IOException("Not a directory");
        }

        String filter = getAbsolutePath(absolutePath).toString();

        if (!recursive) {
            if (!filter.endsWith("/")) {
                filter += "/";
            }
            filter += "[^/]+$";
        }

        try {
            return asyncAm.volumeContents(DOMAIN,
                                          getVolume(),
                                          Integer.MAX_VALUE,
                                          0,
                                          filter,
                                          PatternSemantics.PCRE,
                                          "",
                                          BlobListOrder.LEXICOGRAPHIC,
                                          descending).get().getBlobs();
        } catch (Exception e) {
            throw new IOException(e);
        }
    }

    @Override
    public FileStatus[] listStatus(Path path) throws FileNotFoundException, IOException {
        Path absolutePath = getAbsolutePath(path);
        if (!isDirectory(absolutePath)) {
            return new FileStatus[] { getFileStatus(absolutePath) };
        }
        return listAllSubPaths(path, false, false)
                .stream()
                .map(bd -> asFileStatus(bd))
                .toArray(i -> new FileStatus[i]);
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
        try {
            Path absolutePath = getAbsolutePath(path);
            BlobDescriptor bd = unwindExceptions(() -> asyncAm.statBlob(DOMAIN, getVolume(), absolutePath.toString()).get());
            return asFileStatus(bd);
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

    private FileStatus asFileStatus(BlobDescriptor bd) {
        OwnerGroupInfo owner = null;
        try {
            owner = new OwnerGroupInfo(bd);
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
        boolean isDirectory = isDirectory(bd);
        FsPermission permission = isDirectory ? FsPermission.getDirDefault() : FsPermission.getFileDefault();
        long byteCount = bd.getByteCount();
        return new FileStatus(byteCount, isDirectory, 1, blockSize, getMtime(bd),
                getMtime(bd), permission, owner.getOwner(), owner.getGroup(), new Path(bd.getName()));
    }


    private String getVolume() {
        return uri.getAuthority();
    }

    private void mkDirBlob(Path path) throws IOException {
        try {
            FileStatus fileStatus = getFileStatus(path);
            if (!fileStatus.isDirectory()) {
                throw new IOException("File with same name exists");
            }
        } catch (FileNotFoundException e) {

        }

        String targetPath = path.toString();
        try {
            OwnerGroupInfo owner = OwnerGroupInfo.current();
            Map<String, String> map = new HashMap<>();
            map.put(DIRECTORY_SPECIFIER_KEY, "true");
            map.put(FdsFileSystem.CREATED_BY_USER, owner.getOwner());
            map.put(FdsFileSystem.CREATED_BY_GROUP, owner.getGroup());

            asyncAm.updateBlobOnce(DOMAIN, getVolume(), targetPath, Mode.TRUNCATE.getValue(), ByteBuffer.allocate(0), 0, new ObjectOffset(0), map).get();
        } catch (Exception ex) {
            throw new IOException(ex);
        }
    }

    private boolean isDirectory(BlobDescriptor bd) {
        return bd.getMetadata().containsKey(DIRECTORY_SPECIFIER_KEY);
    }

    private long getMtime(BlobDescriptor bd) {
        if (!bd.getMetadata().containsKey(LAST_MODIFIED_KEY)) {
            return Calendar.getInstance().getTimeInMillis();
        } else {
            String data = bd.getMetadata().get(LAST_MODIFIED_KEY);
            return Long.parseLong(data);
        }
    }

    private int getBlockSize() throws IOException {
        return blockSize;
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

    public Path getAbsolutePath(Path path) {
        path = new Path(path.toString().replaceAll("#.+$", ""));
        if (!path.toString().startsWith("fds://")) {
            return new Path(workingDirectory.toString(), path.toString());
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

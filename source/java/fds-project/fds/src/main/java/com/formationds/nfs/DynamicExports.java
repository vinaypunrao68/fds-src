package com.formationds.nfs;

import com.formationds.xdi.XdiConfigurationApi;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;
import org.dcache.nfs.ExportFile;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.PrintStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.*;
import java.util.stream.Collectors;

public class DynamicExports implements ExportResolver {
    private static final Logger LOG = Logger.getLogger(DynamicExports.class);
    public static final String EXPORTS = "./.exports";
    private XdiConfigurationApi config;
    private Set<String> knownVolumes;
    private Map<String, Integer> exportsByName;
    private Map<Integer, String> exportsById;
    private ExportFile exportFile;

    public DynamicExports(XdiConfigurationApi config) {
        this.config = config;
        knownVolumes = new HashSet<>();
    }

    // TODO/CAVEAT: right now all non-system volumes are exported, will be fixed when we
    // add an NFS volume type.
    private synchronized void refreshOnce() throws IOException {
        Set<String> exportableVolumes = null;
        try {
            exportableVolumes = config.listVolumes(BlockyVfs.DOMAIN)
                    .stream()
                    .filter(v -> !v.getName().startsWith("SYSTEM_VOLUME"))
                    .map(v -> v.getName())
                    .collect(Collectors.toSet());
        } catch (TException e) {
            throw new IOException(e);
        }

        Path path = Paths.get(EXPORTS);

        if (knownVolumes.equals(exportableVolumes) && Files.exists(path)) {
            return;
        }

        Files.deleteIfExists(path);
        PrintStream pw = new PrintStream(new FileOutputStream(EXPORTS));
        exportableVolumes.forEach(v -> pw.println("/" + v + " *(rw,no_root_squash,acl)"));
        pw.close();
        knownVolumes = new HashSet<>(exportableVolumes);
        if (exportFile == null) {
            exportFile = new ExportFile(new File(EXPORTS));
        }
        exportFile.rescan();
        this.exportsByName = exportIds(exportFile);
        this.exportsById = new HashMap<>();
        for (String exportName : exportsByName.keySet()) {
            int id = exportsByName.get(exportName);
            exportsById.put(id, exportName);
        }
    }

    private Map<String, Integer> exportIds(ExportFile exportFile) {
        Map<String, Integer> ids = new HashMap<>();
        exportFile.getExports().forEach(export -> {
            String exportName = new org.apache.hadoop.fs.Path(export.getPath()).getName();
            ids.put(exportName, export.getIndex());
        });
        return ids;
    }

    @Override
    public Collection<String> exportNames() {
        return new HashSet<>(exportsByName.keySet());
    }

    public void start() throws IOException {
        Files.deleteIfExists(Paths.get(EXPORTS));
        refreshOnce();

        new Thread(() -> {
            while (true) {
                try {
                    Thread.sleep(1000);
                    refreshOnce();
                } catch (Exception e) {
                    LOG.error("Error refreshing NFS export file", e);
                }
            }
        }, "NFS export refresher").start();
    }

    public ExportFile exportFile() {
        return exportFile;
    }

    public int exportId(String volumeName) {
        return exportsByName.get(volumeName);
    }

    @Override
    public String volumeName(int volumeId) {
        return exportsById.get(volumeId);
    }

    @Override
    public boolean exists(String volumeName) {
        return exportsByName.containsKey(volumeName);
    }

    @Override
    public int objectSize(String volume) {
        try {
            return config.statVolume(BlockyVfs.DOMAIN, volume).getPolicy().getMaxObjectSizeInBytes();
        } catch (TException e) {
            LOG.error("config.statVolume(" + volume + ") failed", e);
            throw new RuntimeException(e);
        }
    }
}

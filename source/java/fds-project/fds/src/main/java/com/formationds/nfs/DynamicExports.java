package com.formationds.nfs;

import com.formationds.apis.VolumeDescriptor;
import com.formationds.apis.VolumeType;
import com.formationds.util.ConsumerWithException;
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
    private volatile Map<String, Integer> exportsByName;
    private volatile Map<Integer, String> exportsById;
    private final List<ConsumerWithException<String>> volumeDeleteEventHandlers;
    private final ExportFile exportFile;


    public DynamicExports(XdiConfigurationApi config) throws IOException {
        volumeDeleteEventHandlers = new ArrayList<>();
        this.config = config;
        writeExportFile();
        exportFile = new ExportFile(new File(EXPORTS));
        refreshCaches();
    }

    public void refreshOnce() throws IOException {
        Map<Integer, String> deletedExports = new HashMap<>(exportsById);
        writeExportFile();
        exportFile.rescan();
        refreshCaches();
        exportsById.keySet().forEach(k -> deletedExports.remove(k));
        for (String deletedVolume : deletedExports.values()) {
            invokeEventHandlers(deletedVolume);
        }
    }

    private void invokeEventHandlers(String deletedVolume) throws IOException {
        for (ConsumerWithException<String> eventHandler : volumeDeleteEventHandlers) {
            try {
                eventHandler.accept(deletedVolume);
            } catch (Exception e) {
                LOG.error("Error handling deletion of volume " + deletedVolume, e);
                throw new IOException(e);
            }
        }
    }

    private void refreshCaches() {
        Map<String, Integer> exportsByName = new HashMap<>();

        exportFile.getExports().forEach(export -> {
            String exportName = new org.apache.hadoop.fs.Path(export.getPath()).getName();
            exportsByName.put(exportName, export.getIndex());
        });

        HashMap<Integer, String> exportsById = new HashMap<>();
        for (String exportName : exportsByName.keySet()) {
            int id = exportsByName.get(exportName);
            exportsById.put(id, exportName);
        }
        this.exportsByName = exportsByName;
        this.exportsById = exportsById;
    }

    private void writeExportFile() throws IOException {
        Set<VolumeDescriptor> exportableVolumes = null;
        try {
            exportableVolumes = config.listVolumes(XdiVfs.DOMAIN)
                    .stream()
                    .filter(vd -> vd.getPolicy().getVolumeType().equals(VolumeType.NFS))
                    .collect(Collectors.toSet());
        } catch (TException e) {
            throw new IOException(e);
        }

        ExportConfigurationBuilder configBuilder = new ExportConfigurationBuilder();
        deleteExportFile();
        PrintStream pw = new PrintStream(new FileOutputStream(EXPORTS));
        for (VolumeDescriptor exportableVolume : exportableVolumes) {
            String configEntry = configBuilder.buildConfigurationEntry(exportableVolume);
            pw.println(configEntry);
        }
        pw.close();
    }

    public void deleteExportFile() throws IOException {
        Path path = Paths.get(EXPORTS);
        Files.deleteIfExists(path);
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

    public int nfsExportId(String volumeName) {
        return exportsByName.get(volumeName);
    }

    @Override
    public String volumeName(int nfsExportId) {
        return exportsById.get(nfsExportId);
    }

    @Override
    public boolean exists(String volumeName) {
        return exportsByName.containsKey(volumeName);
    }

    @Override
    public int objectSize(String volume) {
        try {
            return config.statVolume(XdiVfs.DOMAIN, volume).getPolicy().getMaxObjectSizeInBytes();
        } catch (TException e) {
            LOG.error("config.statVolume(" + volume + ") failed", e);
            throw new RuntimeException(e);
        }
    }

    @Override
    public long maxVolumeCapacityInBytes(String volume) throws IOException {
        try {
            return config.statVolume(XdiVfs.DOMAIN, volume).getPolicy().getBlockDeviceSizeInBytes();
        } catch (TException e) {
            throw new IOException(e);
        }
    }

    @Override
    public void addVolumeDeleteEventHandler(ConsumerWithException<String> consumer) {
        volumeDeleteEventHandlers.add(consumer);
    }
}

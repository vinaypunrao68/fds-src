package com.formationds.nfs;

import com.formationds.apis.VolumeDescriptor;
import com.formationds.apis.VolumeType;
import com.formationds.protocol.NfsOption;
import com.formationds.xdi.XdiConfigurationApi;
import com.google.common.base.Joiner;
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
    private final ExportFile exportFile;


    public DynamicExports(XdiConfigurationApi config) throws IOException {
        this.config = config;
        writeExportFile();
        exportFile = new ExportFile(new File(EXPORTS));
        refreshCaches();
    }

    private void refreshOnce() throws IOException {
        writeExportFile();
        exportFile.rescan();
        refreshCaches();
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

        Path path = Paths.get(EXPORTS);
        Files.deleteIfExists(path);
        PrintStream pw = new PrintStream(new FileOutputStream(EXPORTS));
        exportableVolumes.forEach(vd -> pw.println(buildExportOptions(vd)));
        pw.close();
    }

    private String buildExportOptions(VolumeDescriptor vd) {
        NfsOption nfsOptions = vd.getPolicy().getNfsOptions();
        String optionsClause = nfsOptions.getOptions();
        // Remove the yet-unsupported async option
        StringTokenizer tokenizer = new StringTokenizer(optionsClause, ",", false);
        Set<String> options = new HashSet<>();
        options.add("rw");
        while (tokenizer.hasMoreTokens()) {
            String token = tokenizer.nextToken();
            if (!token.contains("sync")) {
                options.add(token);
            }
        }
        optionsClause = "/" + vd.getName() + " " + nfsOptions.getClient() + "(" + Joiner.on(",").join(options) + ")";
        return optionsClause;
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
            return config.statVolume(XdiVfs.DOMAIN, volume).getPolicy().getMaxObjectSizeInBytes();
        } catch (TException e) {
            LOG.error("config.statVolume(" + volume + ") failed", e);
            throw new RuntimeException(e);
        }
    }
}

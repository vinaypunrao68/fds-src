package com.formationds.nfs;

import org.apache.hadoop.fs.Path;
import org.dcache.nfs.ExportFile;

import java.util.HashMap;
import java.util.Map;

class ExportResolver {
    private final Map<String, Integer> map;

    public ExportResolver(ExportFile file) {
        map = new HashMap<>();
        file.getExports().forEach(export -> {
            String exportName = new Path(export.getPath()).getName();
            map.put(exportName, export.getIndex());
        });
    }

    public int exportId(String volumeName) {
        return map.get(volumeName);
    }
}

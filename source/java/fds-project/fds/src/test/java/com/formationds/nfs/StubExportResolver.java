package com.formationds.nfs;

public class StubExportResolver implements ExportResolver {
    private static final int EXPORT_ID = 1;
    private String volumeName;
    private int objectSize;

    public StubExportResolver(String volumeName, int objectSize) {
        this.volumeName = volumeName;
        this.objectSize = objectSize;
    }

    @Override
    public int exportId(String volumeName) {
        if (volumeName.equals(this.volumeName)) {
            return EXPORT_ID;
        } else {
            throw new RuntimeException("No such volume");
        }
    }

    @Override
    public String volumeName(int volumeId) {
        if (volumeId == EXPORT_ID) {
            return volumeName;
        } else {
            throw new RuntimeException("No such volume");
        }
    }

    @Override
    public boolean exists(String volumeName) {
        return this.volumeName.equals(volumeName);
    }

    @Override
    public int objectSize(String volume) {
        return objectSize;
    }
}

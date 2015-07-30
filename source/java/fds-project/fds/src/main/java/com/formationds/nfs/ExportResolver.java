package com.formationds.nfs;

public interface ExportResolver {
    public int exportId(String volumeName);

    public String volumeName(int volumeId);

    public boolean exists(String volumeName);

    public int objectSize(String volume);
}

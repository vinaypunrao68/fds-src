package com.formationds.nfs;

import com.formationds.util.ConsumerWithException;

import java.util.Collection;

public interface ExportResolver {
    Collection<String> exportNames();

    public int exportId(String volumeName);
    public String volumeName(int volumeId);
    public boolean exists(String volumeName);
    public int objectSize(String volume);

    void addVolumeDeleteEventHandler(ConsumerWithException<String> consumer);
}

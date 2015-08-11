package com.formationds.nfs;

import org.glassfish.grizzly.memory.HeapBuffer;
import org.glassfish.grizzly.memory.MemoryManager;
import org.glassfish.grizzly.memory.MemoryProbe;
import org.glassfish.grizzly.monitoring.DefaultMonitoringConfig;
import org.glassfish.grizzly.monitoring.MonitoringConfig;

public class FdsMemoryManager implements MemoryManager<HeapBuffer> {

    public FdsMemoryManager() {
        System.out.println("foo");
    }

    @Override
    public HeapBuffer allocate(int size) {
        return HeapBuffer.wrap(new byte[size]);
    }

    @Override
    public HeapBuffer allocateAtLeast(int size) {
        return HeapBuffer.wrap(new byte[size]);
    }

    @Override
    public HeapBuffer reallocate(HeapBuffer oldBuffer, int newSize) {
        int position = oldBuffer.position();
        oldBuffer.position(0);
        HeapBuffer newBuffer = allocate(newSize);
        newBuffer.put(oldBuffer);
        newBuffer.position(position);
        return newBuffer;
    }

    @Override
    public void release(HeapBuffer buffer) {

    }

    @Override
    public boolean willAllocateDirect(int size) {
        return false;
    }

    @Override
    public MonitoringConfig<MemoryProbe> getMonitoringConfig() {
        return new DefaultMonitoringConfig<>(MemoryProbe.class);
    }
}

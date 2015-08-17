package com.formationds.nfs;

import java.nio.ByteBuffer;
import java.util.Map;

public class ObjectView {
    private Map<String, String> map;
    private ByteBuffer buf;

    public ObjectView(Map<String, String> map, ByteBuffer buf) {
        this.map = map;
        this.buf = buf;
    }

    public Map<String, String> getMetadata() {
        return map;
    }

    public ByteBuffer getBuf() {
        return buf;
    }
}

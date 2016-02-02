package com.formationds.nfs;

import java.util.Map;

public class ObjectAndMetadata {
    private Map<String, String> map;
    private FdsObject fdsObject;

    public ObjectAndMetadata(Map<String, String> map, FdsObject fdsObject) {
        this.map = map;
        this.fdsObject = fdsObject;
    }

    public Map<String, String> getMetadata() {
        return map;
    }

    public FdsObject getFdsObject() {
        return fdsObject;
    }
}

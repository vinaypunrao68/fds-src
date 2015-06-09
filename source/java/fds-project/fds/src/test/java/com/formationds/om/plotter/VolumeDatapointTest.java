package com.formationds.om.plotter;

import org.joda.time.DateTime;
import org.json.JSONObject;
import org.junit.Test;

import static org.junit.Assert.assertEquals;

public class VolumeDatapointTest {
    @Test
    public void testFromJson() {
        JSONObject o = new JSONObject()
                .put("timestamp", "12")
                .put("volume", "documents")
                .put("key", "size")
                .put("value", "42");
        VolumeDatapoint volumeDatapoint = new VolumeDatapoint(o);
        assertEquals("documents", volumeDatapoint.getVolumeName());
        assertEquals("size", volumeDatapoint.getKey());
        assertEquals(42d, volumeDatapoint.getValue(), .001);
        assertEquals(new DateTime(12), volumeDatapoint.getDateTime());
    }
}
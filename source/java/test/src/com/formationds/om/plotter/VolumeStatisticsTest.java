package com.formationds.om.plotter;

import com.google.common.base.Joiner;
import org.joda.time.DateTime;
import org.joda.time.Duration;
import org.junit.Test;

import static org.junit.Assert.assertEquals;

public class VolumeStatisticsTest {
    @Test
    public void testWindow() {
        VolumeStatistics volumeStatistics = new VolumeStatistics(Duration.standardSeconds(20));
        assertEquals(0, volumeStatistics.size());
        DateTime now = new DateTime(0);
        volumeStatistics.put(new VolumeDatapoint(now, "foo", "age", 12));
        assertEquals(1, volumeStatistics.size());
        now = now.plusSeconds(40);
        volumeStatistics.put(new VolumeDatapoint(now, "foo", "age", 13));
        assertEquals(1, volumeStatistics.size());
        volumeStatistics.put(new VolumeDatapoint(now, "bar", "age", 13));
        assertEquals(2, volumeStatistics.size());

        assertEquals("bar, foo", Joiner.on(", ").join(volumeStatistics.volumes()));
    }
}
package com.formationds.am;

import com.formationds.streaming.DataPointPair;
import com.formationds.streaming.volumeDataPoints;
import com.google.common.collect.Lists;
import org.json.JSONArray;
import org.junit.Test;

import java.util.ArrayList;
import java.util.List;

import static org.junit.Assert.assertEquals;

public class JsonStatisticsFormatterTest {

    @Test
    public void testFormat() {
        List<volumeDataPoints> datapoints = new ArrayList<>();
        datapoints.add(new volumeDataPoints("Panda", 42, Lists.newArrayList(new DataPointPair("size", 12), new DataPointPair("age", 14))));
        datapoints.add(new volumeDataPoints("Lemur", 43, Lists.newArrayList(new DataPointPair("size", 42))));
        JSONArray array = new JsonStatisticsFormatter().format(datapoints);
        assertEquals(expected, array.toString(4));
    }

    private static final String expected =
            "[\n" +
                    "    {\n" +
                    "        \"volume\": \"Panda\",\n" +
                    "        \"value\": \"12.0\",\n" +
                    "        \"key\": \"size\",\n" +
                    "        \"timestamp\": \"42\"\n" +
                    "    },\n" +
                    "    {\n" +
                    "        \"volume\": \"Panda\",\n" +
                    "        \"value\": \"14.0\",\n" +
                    "        \"key\": \"age\",\n" +
                    "        \"timestamp\": \"42\"\n" +
                    "    },\n" +
                    "    {\n" +
                    "        \"volume\": \"Lemur\",\n" +
                    "        \"value\": \"42.0\",\n" +
                    "        \"key\": \"size\",\n" +
                    "        \"timestamp\": \"43\"\n" +
                    "    }\n" +
                    "]";
}
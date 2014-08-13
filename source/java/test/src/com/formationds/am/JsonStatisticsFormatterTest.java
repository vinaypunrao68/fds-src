package com.formationds.am;

import org.junit.Test;

public class JsonStatisticsFormatterTest {

    @Test
    public void testFormat() {
    }

    private static final String expected =
            "[\n" +
                    "    {\n" +
                    "        \"timestamp\" : \"Tue Aug  5 21:22:19 UTC 2014\",\n" +
                    "        \"volume\" : \"Documents\",\n" +
                    "        \"key\" : \"size\",\n" +
                    "        \"value\" : \"5368709120\"\n" +
                    "    },\n" +
                    "    {\n" +
                    "        \"timestamp\" : \"Tue Aug  5 21:23:19 UTC 2014\",\n" +
                    "        \"volume\" : \"Documents\",\n" +
                    "        \"key\" : \"size\",\n" +
                    "        \"value\" : \"5368720120\"\n" +
                    "    },\n" +
                    "    {\n" +
                    "        \"timestamp\" : \"Tue Aug  5 21:22:19 UTC 2014\",\n" +
                    "        \"volume\" : \"Source\",\n" +
                    "        \"key\" : \"size\",\n" +
                    "        \"value\" : \"5368709120\"\n" +
                    "    }\n" +
                    "]";
}
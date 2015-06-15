package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.amazonaws.util.StringInputStream;
import com.google.common.base.Joiner;
import com.google.common.collect.Lists;
import org.junit.Test;

import java.util.Collection;

import static org.junit.Assert.assertEquals;

public class DeleteObjectsFrameTest {
    @Test
    public void testParseFrame() throws Exception {
        Collection<String> keys = Lists.newArrayList();
        DeleteObjectsFrame frame = new DeleteObjectsFrame(new StringInputStream(input), s -> keys.add(s));
        frame.parse();
        assertEquals(3, keys.size());
        assertEquals("foo, bar, hello", Joiner.on(", ").join(keys));
    }

    private static final String input =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
            "<Delete>\n" +
            "    <Quiet>true</Quiet>\n" +
            "    <Object>\n" +
            "         <Key>foo</Key>\n" +
            "         <VersionId>VersionId</VersionId>\n" +
            "    </Object>\n" +
            "    <Object>\n" +
            "         <Key>bar</Key>\n" +
            "    </Object>\n" +
            "    <Object>\n" +
            "         <Key>hello</Key>\n" +
            "    </Object>\n" +
            "</Delete>\n";
}

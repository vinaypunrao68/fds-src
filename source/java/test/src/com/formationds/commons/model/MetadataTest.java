/**
 * Copyright (c) 2014 Formation Data Systems.
 * All rights reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.helper.ObjectModelHelper;
import org.junit.Assert;
import org.junit.Test;

public class MetadataTest {
    @Test
    public void createTest() {
        final Metadata md = new Metadata();

        final long time = System.currentTimeMillis();

        md.setKey("Volume Value Key");
        md.setTimestamp(time);
        md.setValue("Volume Value Key Value");
        md.setVolume("Volume Name");

        System.out.println(md.toString());

        Assert.assertEquals("Key don't match", md.getKey(), "Volume Value Key");
        Assert.assertEquals("Timestamp don't match", md.getTimestamp(), time);
        Assert.assertEquals("Value don't match",
                md.getValue(),
                "Volume Value Key Value");
        Assert.assertEquals("Name don't match", md.getVolume(), "Volume Name");

        System.out.println(ObjectModelHelper.toJSON(md));
    }
}
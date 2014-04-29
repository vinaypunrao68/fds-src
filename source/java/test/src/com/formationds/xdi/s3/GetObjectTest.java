package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.BlobDescriptor;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.Xdi;
import com.google.common.collect.Maps;
import org.apache.commons.codec.binary.Hex;
import org.eclipse.jetty.server.Request;
import org.junit.Test;

import java.nio.ByteBuffer;

import static org.junit.Assert.assertEquals;
import static org.mockito.Matchers.anyString;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

public class GetObjectTest {

    public static final String IF_NONE_MATCH = "If-None-Match";

    @Test
    public void testIfMatch() throws Exception {
        Xdi xdi = mock(Xdi.class);
        byte[] digest = new byte[]{1, 2, 3};
        BlobDescriptor descriptor = new BlobDescriptor("poop", 0, ByteBuffer.wrap(digest), Maps.newHashMap());
        when(xdi.statBlob(anyString(), anyString(), anyString())).thenReturn(descriptor);
        GetObject handler = new GetObject(xdi);
        Request request = mock(Request.class);
        when(request.getHeader(IF_NONE_MATCH)).thenReturn(null);
        Resource resource = handler.handle(request, "poop", "panda");
        assertEquals(200, resource.getHttpStatus());

        byte[] nonMatchingDigest = {42};
        when(request.getHeader(IF_NONE_MATCH)).thenReturn(Hex.encodeHexString(nonMatchingDigest));
        resource = handler.handle(request, "poop", "panda");
        assertEquals(200, resource.getHttpStatus());

        when(request.getHeader(IF_NONE_MATCH)).thenReturn(Hex.encodeHexString(digest));
        resource = handler.handle(request, "poop", "panda");
        assertEquals(304, resource.getHttpStatus());
    }
}

package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.formationds.xdi.VolumeInfo;
import com.formationds.xdi.Xdi;
import org.eclipse.jetty.server.Request;

import javax.servlet.http.HttpServletResponse;
import java.util.Map;

public class StatBucket implements RequestHandler {
    private Xdi xdi;

    public StatBucket(Xdi xdi) {
        this.xdi = xdi;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String bucketName = requiredString(routeParameters, "bucket");
        try {
            VolumeInfo i = xdi.statVolume(bucketName);
            return new TextResource("");
        } catch (Exception e) {
            return new TextResource(HttpServletResponse.SC_NOT_FOUND, e.getMessage());
        }
    }
}

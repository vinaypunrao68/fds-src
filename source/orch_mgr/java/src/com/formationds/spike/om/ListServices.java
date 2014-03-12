package com.formationds.spike.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.google.common.base.Joiner;

import javax.servlet.http.HttpServletRequest;

public class ListServices implements RequestHandler {
    private ServiceDirectory serviceDirectory;

    public ListServices(ServiceDirectory serviceDirectory) {
        this.serviceDirectory = serviceDirectory;
    }

    @Override
    public Resource handle(HttpServletRequest request) throws Exception {
        String[] nodeDescriptions = serviceDirectory
                .allNodes()
                .map(n -> "    {name: '" + n.getNode_name() + "', uuid:'" + n.getNode_uuid().toString() + "', type:'" + n.getNode_type() + "'}")
                .toArray(i -> new String[i]);
        return new TextResource("[\n" + Joiner.on(",\n").join(nodeDescriptions) + "\n]");
    }
}

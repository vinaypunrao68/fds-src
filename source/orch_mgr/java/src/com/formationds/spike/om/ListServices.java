package com.formationds.spike.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.spike.ServiceDirectory;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.google.common.base.Joiner;
import org.eclipse.jetty.server.Request;

import java.util.Map;

public class ListServices implements RequestHandler {
    private ServiceDirectory serviceDirectory;

    public ListServices(ServiceDirectory serviceDirectory) {
        this.serviceDirectory = serviceDirectory;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String[] nodeDescriptions = serviceDirectory
                .allNodes()
                .map(n -> "    {name: '" + n.getNode_name() + "', uuid:'" + n.getNode_uuid().toString() + "', type:'" + n.getNode_type() + "'}")
                .toArray(i -> new String[i]);
        return new TextResource("[\n" + Joiner.on(",\n").join(nodeDescriptions) + "\n]");
    }
}

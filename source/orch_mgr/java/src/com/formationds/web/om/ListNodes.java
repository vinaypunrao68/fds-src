package com.formationds.web.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.om.NativeApi;
import com.formationds.util.MutableAcceptor;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.google.common.base.Joiner;

import javax.servlet.http.HttpServletRequest;

public class ListNodes implements RequestHandler {
    @Override
    public Resource handle(HttpServletRequest request) throws Exception {
        MutableAcceptor<String> acceptor = new MutableAcceptor<>();
        NativeApi.listNodes(acceptor);
        if (acceptor.size() == 0) {
           return new TextResource("No SM nodes yet");
        } else {
           return new TextResource("[" + Joiner.on(", ").join(acceptor) + "]");
        }
    }
}

package com.formationds.web.toolkit;

import javax.servlet.http.HttpServletRequest;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
public class FourOhFour extends TextResource implements RequestHandler  {
    public FourOhFour() {
        super(404, "Four, oh, four.  Resource not found.");
    }

    @Override
    public Resource handle(HttpServletRequest request) throws Exception {
        return this;
    }
}

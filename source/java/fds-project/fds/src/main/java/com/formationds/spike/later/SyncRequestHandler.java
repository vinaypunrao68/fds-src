package com.formationds.spike.later;

import com.formationds.web.toolkit.Resource;

public interface SyncRequestHandler {
    public Resource handle(HttpContext context) throws Exception;
}

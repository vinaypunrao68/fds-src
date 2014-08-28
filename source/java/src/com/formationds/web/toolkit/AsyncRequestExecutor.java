package com.formationds.web.toolkit;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.eclipse.jetty.server.Request;
import org.eclipse.jetty.server.Response;
import java.util.Optional;
import java.util.concurrent.CompletableFuture;

public interface AsyncRequestExecutor {
    public Optional<CompletableFuture<Void>> tryExecuteRequest(Request request, Response response);
}

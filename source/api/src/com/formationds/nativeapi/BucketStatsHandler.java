package com.formationds.nativeapi;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import java.util.Collection;

public interface BucketStatsHandler {
    void handle(String timestamp, Collection<BucketStatsContent> contents);
}

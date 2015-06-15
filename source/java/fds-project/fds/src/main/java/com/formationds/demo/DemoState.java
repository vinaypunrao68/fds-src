package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import java.util.Optional;

public interface DemoState {
    void setSearchExpression(String searchExpression);

    Optional<String> getCurrentSearchExpression();

    Optional<ImageResource> peekReadQueue();

    Optional<ImageResource> peekWriteQueue();

    void setObjectStore(ObjectStoreType type);

    ObjectStoreType getObjectStore();

    int getReadThrottle();

    void setReadThrottle(int value);

    int getWriteThrottle();

    void setWriteThrottle(int value);

    BucketStats readCounts();

    BucketStats writeCounts();
}

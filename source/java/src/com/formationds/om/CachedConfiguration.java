package com.formationds.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */


import com.formationds.apis.User;
import org.apache.thrift.TException;

import java.util.Map;

public interface CachedConfiguration {
    Map<String, User> usersByLogin() throws TException;
}

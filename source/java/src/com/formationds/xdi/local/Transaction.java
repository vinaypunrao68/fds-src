package com.formationds.xdi.local;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.hibernate.Session;

public interface Transaction<T> {
    public T run(Session session);
}

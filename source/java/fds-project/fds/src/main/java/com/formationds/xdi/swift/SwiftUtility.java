package com.formationds.xdi.swift;/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.commons.util.DateTimeFormats;
import com.formationds.web.toolkit.Resource;
import org.joda.time.DateTime;
import org.joda.time.format.DateTimeFormat;
import org.joda.time.format.DateTimeFormatter;

import java.util.UUID;

public class SwiftUtility {

    public static Resource swiftResource(Resource res) {
        return swiftResource(res, UUID.randomUUID());
    }

    public static Resource swiftResource(Resource res, UUID transactionId) {
        return res.withHeader("X-Trans-Id", transactionId.toString())
                .withHeader("Date", formatRfc1123Date(DateTime.now()));
    }

    public static String formatRfc1123Date(DateTime date) {
        return DateTimeFormats.RFC1123().print(date);
    }
}

package com.formationds.xdi.swift;/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

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
        // TODO: put this formatter somewhere generally accessible
        DateTimeFormatter rfc1123DateFormat = DateTimeFormat.forPattern("EEE, dd MMM yyyy HH:mm:ss 'GMT'").withZoneUTC();
        return rfc1123DateFormat.print(date);
    }
}

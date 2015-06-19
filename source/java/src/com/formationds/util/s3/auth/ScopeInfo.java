package com.formationds.util.s3.auth;

import org.joda.time.DateTime;
import org.joda.time.DateTimeZone;
import org.joda.time.format.ISODateTimeFormat;

public class ScopeInfo {
    public String dateStamp;
    public String service;
    public String region;

    public static ScopeInfo parse(String scopeString) {
        String[] scopeParts = scopeString.split("/");
        if(scopeParts.length != 4)
            throw new IllegalArgumentException("Invalid credential scope [" + scopeString + "] - expecting 5 slash delimited parts");
        if(!scopeParts[3].equals("aws4_request"))
            throw new IllegalArgumentException("Invalid credential scope [" + scopeString + "] - expecting last part to be 'aws4_request'");

        return new ScopeInfo(scopeParts[0], scopeParts[1], scopeParts[2]);
    }

    public ScopeInfo(String dateStamp, String region, String service) {
        this.dateStamp = dateStamp;
        this.service = service;
        this.region = region;
    }

    public ScopeInfo(DateTime date, String region, String service) {
        this(date.toDateTime(DateTimeZone.UTC).toString(ISODateTimeFormat.basicDate()), region, service);
    }

    public String toScopeString() {
        return String.join("/", dateStamp, region, service, "aws4_request");
    }

    public String getDateStamp() {
        return dateStamp;
    }

    public String getService() {
        return service;
    }

    public String getRegion() {
        return region;
    }
}

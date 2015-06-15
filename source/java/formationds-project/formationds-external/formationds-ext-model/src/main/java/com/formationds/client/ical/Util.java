/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.ical;

import java.text.DateFormat;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;

public class Util {
    private static final DateFormat ISO_DATE =
        new SimpleDateFormat( "yyyyMMdd'T'HHmmss" );

    /**
     * @param date the {@link String} representing the RDATE spec format
     *
     * @return Returns {@link Date}
     *
     * @throws ParseException if there is a parse error
     */
    public static Date toiCalFormat( final String date )
        throws ParseException {
        return fromISODateTime( date );
    }

    /**
     * @param date the {@link Date} representing the date
     *
     * @return Returns {@link String} representing the ISO Date format of
     * "yyyyMMddTHHmmssZ"
     */
    public static String isoDateTimeUTC( final Date date ) {
        return ISO_DATE.format( date );
    }

    /**
     * @param date the {@link String} representing the ISO Date format of
     *             "yyyyMMddThhmmss"
     *
     * @return Date Returns {@link Date} representing the {@code date}
     */
    public static Date fromISODateTime( final String date )
        throws ParseException {
        return ISO_DATE.parse( date );
    }

}

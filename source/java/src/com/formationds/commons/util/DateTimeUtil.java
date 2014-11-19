/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.util;

import java.time.Instant;
import java.time.LocalDateTime;
import java.time.ZoneId;
import java.time.ZonedDateTime;
import java.time.format.DateTimeFormatter;

/**
 * @author ptinius
 */
public class DateTimeUtil {

    /**
     * @param year the {@link Integer} representing the year
     *
     * @return Returns {@code true} if {@code year} is a leap tear. Otherwise,
     * {@code false} is returned
     */
    public static boolean isLeapYear( final Integer year ) {
        return ( year % 4 == 0 ) &&
               ( ( year % 100 != 0 ) ||
                 ( year % 400 == 0 ) );
    }

    /**
     * @param year the {@link Integer} representing the year
     *
     * @return Returns {@link Integer} representing the length of {@code year}
     */
    public static int yearLength( final Integer year ) {
        return isLeapYear( year ) ? 366 : 365;
    }

    /**
     * @param epoch the {@code long} representing the number of seconds since
     *              1970-01-01T00:00:00Z ( epoch )
     *
     * @return Returns {@code long} representing
     */
    public static Long epochToMilliseconds( final long epoch ) {
        return Instant.ofEpochSecond( epoch )
                      .toEpochMilli();
    }

    /**
     * @param ldt the {@link java.time.LocalDateTime}
     *
     * @return Returns {@link Long} representing epoch as seconds
     */
    public static Long toUnixEpoch( final LocalDateTime ldt ) {
        return ZonedDateTime.of( ldt, ZoneId.systemDefault() )
                            .toEpochSecond();
    }

    /**
     * @param date the {@link String} representing the date
     * @param pattern the {!@link String} representing the pattern
     *
     * @return Returns the {@link java.time.LocalDateTime}
     */
    public static LocalDateTime parse( final String date,
                                       final String pattern ) {
        return LocalDateTime.parse( date,
                                    DateTimeFormatter.ofPattern( pattern ) );
    }
}

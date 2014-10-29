/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.commons.model.type.iCalFrequency;
import com.formationds.commons.model.type.iCalKeys;
import com.formationds.commons.model.type.iCalWeekDays;
import com.formationds.commons.model.util.RRIntegerValidator;
import com.formationds.commons.util.Numbers;
import com.formationds.commons.util.WeekDays;
import com.google.gson.annotations.SerializedName;

import java.text.ParseException;
import java.util.Date;
import java.util.NoSuchElementException;
import java.util.StringTokenizer;

/**
 * @author ptinius
 */
@SuppressWarnings( "UnusedDeclaration" )
public class RecurrenceRule
    extends ModelBase {
    private static final long serialVersionUID = -6056637443021228929L;

    private static final String MISSING_TOKEN =
        "Missing expected token, last token: '%s'";

    private static final String RRULE_STARTSWITH = "RRULE:";


    @SerializedName( "FREQ" )
    private String frequency;
    @SerializedName( "UNTIL" )
    private Date until;
    @SerializedName( "COUNT" )
    private Integer count = null;
    @SerializedName( "INTERVAL" )
    private Integer interval = null;

    @SerializedName( "BYSECOND" )
    private Numbers<Integer> seconds = null;
    @SerializedName( "BYMINUTE" )
    private Numbers<Integer> minutes = null;
    @SerializedName( "BYHOUR" )
    private Numbers<Integer> hours = null;
    @SerializedName( "BYMONTHDAY" )
    private Numbers<Integer> monthDays = null;
    @SerializedName( "BYYEAR" )
    private Numbers<Integer> yearDays = null;
    @SerializedName( "BYWEEKNO" )
    private Numbers<Integer> weekNo = null;
    @SerializedName( "BYMONTH" )
    private Numbers<Integer> months = null;
    @SerializedName( "BYSETPOS" )
    private Numbers<Integer> position = null;

    @SerializedName( "WKST" )
    private String weekStartDay;

    @SerializedName( "BYDAY" )
    private WeekDays<iCalWeekDays> days = null;

    /**
     * default constructor
     */
    public RecurrenceRule() {
    }

    /**
     * @param frequency the {@link String} representing the frequency
     * @param until     the {@link java.util.Date} defines a date-time value
     *                  which bounds the recurrence rule in an inclusive
     *                  manner
     */
    public RecurrenceRule( final String frequency, final Date until ) {
        setFrequency( frequency );
        setUntil( until );
    }

    /**
     * @param frequency the {@link String} representing the frequency
     * @param count     the {@link Integer} defines the number of occurrences
     *                  at which to range-bound the recurrence
     */
    public RecurrenceRule( final String frequency, final Integer count ) {
        setFrequency( frequency );
        setCount( count );
    }

    /**
     * @return Returns {@link String} representing the iCAL Frequency
     */
    public String getFrequency() {
        return frequency;
    }

    public void setFrequency( final String frequency ) {
        this.frequency = frequency;
        validate();
    }

    public Integer getInterval() {
        return interval;
    }

    public void setInterval( final Integer interval ) {
        this.interval = interval;
    }

    public Integer getCount() {
        return count;
    }

    public void setCount( final Integer count ) {
        this.count = count;
    }

    public Date getUntil() {
        return until;
    }

    public void setUntil( final Date until ) {
        this.until = until;
    }

    public String getWeekStartDay() {
        return weekStartDay;
    }

    public void setWeekStartDay( final String weekStartDay ) {
        this.weekStartDay = weekStartDay;
    }

    public Numbers<Integer> getPosition() {
        return position;
    }

    public void setPosition( final Numbers<Integer> position ) {
        this.position = position;
    }

    public Numbers<Integer> getMonths() {
        return months;
    }

    public void setMonths( final Numbers<Integer> months ) {
        this.months = months;
    }

    public Numbers<Integer> getWeekNo() {
        return weekNo;
    }

    public void setWeekNo( final Numbers<Integer> weekNo ) {
        this.weekNo = weekNo;
    }

    public Numbers<Integer> getYearDays() {
        return yearDays;
    }

    public void setYearDays( final Numbers<Integer> yearDays ) {
        this.yearDays = yearDays;
    }

    public Numbers<Integer> getMonthDays() {
        return monthDays;
    }

    public void setMonthDays( final Numbers<Integer> monthDays ) {
        this.monthDays = monthDays;
    }

    public WeekDays<iCalWeekDays> getDays() {
        return days;
    }

    public void setDays( final WeekDays<iCalWeekDays> days ) {
        this.days = days;
    }

    public Numbers<Integer> getMinutes() {
        return minutes;
    }

    public void setMinutes( final Numbers<Integer> minutes ) {
        this.minutes = minutes;
    }

    public Numbers<Integer> getHours() {
        return hours;
    }

    public void setHours( final Numbers<Integer> hours ) {
        this.hours = hours;
    }

    public Numbers<Integer> getSeconds() {
        return seconds;
    }

    public void setSeconds( final Numbers<Integer> seconds ) {
        this.seconds = seconds;
    }

    /**
     * @param rrule the {@link String} representing the iCal Recurrence Rule
     *
     * @return Returns the {@link RecurrenceRule}
     *
     * @throws java.text.ParseException any parsing error
     */
    public RecurrenceRule parser( final String rrule )
        throws ParseException {

        final RecurrenceRule RRule = new RecurrenceRule();
        final StringTokenizer t = new StringTokenizer( rrule, ";=" );

        while( t.hasMoreTokens() ) {
            String token = t.nextToken();
            if( token.startsWith( RRULE_STARTSWITH ) ) {
                token = token.replace( RRULE_STARTSWITH, "" )
                             .trim();
            }

            final iCalKeys key = iCalKeys.valueOf( token );
            if( iCalKeys.FREQ.equals( key ) ) {
                RRule.setFrequency( nextToken( t, token ) );
            } else {
                switch( key ) {
                    case UNTIL:
                        final String untilString = nextToken( t, token );
                        RRule.setUntil( ObjectModelHelper.toiCalFormat( untilString ) );
                        break;
                    case COUNT:
                        RRule.setCount( Integer.parseInt( nextToken( t, token ) ) );
                        break;
                    case INTERVAL:
                        RRule.setInterval( Integer.parseInt( nextToken( t, token ) ) );
                        break;
                    case WKST:
                        weekStartDay = nextToken( t, token );
                        RRule.setWeekStartDay( weekStartDay );
                        break;
                    case BYSECOND:
                        if( seconds == null ) {
                            seconds = new Numbers<>( );
                            seconds.validator( new RRIntegerValidator( 0, 59, false ) );
                        }
                        seconds.add( nextToken( t, token ), "," );
                        RRule.setSeconds( seconds );
                        break;
                    case BYMINUTE:
                        if( minutes == null ) {
                            minutes = new Numbers<>( );
                            minutes.validator( new RRIntegerValidator( 0, 59, false ) );
                        }
                        minutes.add( nextToken( t, token ), "," );
                        RRule.setMinutes( minutes );
                        break;
                    case BYHOUR:
                        if( hours == null ) {
                            hours = new Numbers<>( );
                            hours.validator( new RRIntegerValidator( 0, 23, false ) );
                        }
                        hours.add( nextToken( t, token ), "," );
                        RRule.setHours( hours );
                        break;
//                    TODO fix serialization issue with enum iCalWeekDays
//                    case BYDAY:
//                        if( days == null ) {
//                            days = new WeekDays<>( );
//                        }
//                        days.add( nextToken( t, token ), "," );
//                        RRule.setDays( days );
//                        break;
                    case BYWEEKNO:
                        if( weekNo == null ) {
                            weekNo = new Numbers<>( );
                            weekNo.validator( new RRIntegerValidator( 1, 53, true ) );
                        }
                        weekNo.add( nextToken( t, token ), "," );
                        RRule.setWeekNo( weekNo );
                        break;
                    case BYMONTHDAY:
                        if( monthDays == null ) {
                            monthDays = new Numbers<>( );
                            monthDays.validator( new RRIntegerValidator( 1, 31, true ) );
                        }
                        monthDays.add( nextToken( t, token ), "," );
                        RRule.setMonthDays( monthDays );
                        break;
                    case BYMONTH:
                        if( months == null ) {
                            months = new Numbers<>( );
                            months.validator( new RRIntegerValidator( 1, 12, false ) );
                        }
                        months.add( nextToken( t, token ), "," );
                        RRule.setMonths( months );
                        break;
                    case BYYEARDAY:
                        if( yearDays == null ) {
                            yearDays = new Numbers<>( );
                            yearDays.validator( new RRIntegerValidator( 1, 366, true ) );
                        }
                        yearDays.add( nextToken( t, token ), "," );
                        RRule.setYearDays( yearDays );
                        break;
                    case BYSETPOS:
                        if( position == null ) {
                            position = new Numbers<>( );
                        }
                        position.add( nextToken( t, token ), "," );
                        RRule.setPosition( position );
                        break;
                    // SKIP -- assuming its a experimental value.
                }
            }
        }


        return RRule;
    }

    private String nextToken( StringTokenizer t, String lastToken ) {
        try {
            return t.nextToken();
        } catch( NoSuchElementException e ) {
            throw new IllegalArgumentException( String.format( MISSING_TOKEN,
                                                               lastToken ) );
        }
    }

    private void validate() {
        iCalFrequency.valueOf( getFrequency() );
    }

    /**
     * @return Returns {@link String} representing the recurrence rule
     */
    @Override
    public final String toString() {
        final StringBuilder b = new StringBuilder();

        b.append( iCalKeys.FREQ );
        b.append( '=' );
        b.append( frequency );

        if( weekStartDay != null ) {
            b.append( ';' );
            b.append( iCalKeys.WKST );
            b.append( '=' );
            b.append( weekStartDay );
        }

        if( until != null ) {
            b.append( ';' );
            b.append( iCalKeys.UNTIL );
            b.append( '=' );
            b.append( ObjectModelHelper.isoDateTimeUTC( until ) );
        }

        if( count != null && count >= 1 ) {
            b.append( ';' );
            b.append( iCalKeys.COUNT );
            b.append( '=' );
            b.append( count );
        }

        if( interval != null && interval >= 1 ) {
            b.append( ';' );
            b.append( iCalKeys.INTERVAL );
            b.append( '=' );
            b.append( interval );
        }

        if( months != null && !months.isEmpty() ) {
            b.append( ';' );
            b.append( iCalKeys.BYMONTH );
            b.append( '=' );
            b.append( months );
        }
        if( weekNo != null && !weekNo.isEmpty() ) {
            b.append( ';' );
            b.append( iCalKeys.BYWEEKNO );
            b.append( '=' );
            b.append( weekNo );
        }
// TODO fix serialization issue with enum iCalWeekDays
//        if( days != null && !days.isEmpty() ) {
//            b.append( ';' );
//            b.append( iCalKeys.BYDAY );
//            b.append( '=' );
//            b.append( days );
//        }

        if( yearDays != null && !yearDays.isEmpty() ) {
            b.append( ';' );
            b.append( iCalKeys.BYYEARDAY );
            b.append( '=' );
            b.append( yearDays );
        }

        if( monthDays != null && !monthDays.isEmpty() ) {
            b.append( ';' );
            b.append( iCalKeys.BYMONTHDAY );
            b.append( '=' );
            b.append( monthDays );
        }

        if( hours != null && !hours.isEmpty() ) {
            b.append( ';' );
            b.append( iCalKeys.BYHOUR );
            b.append( '=' );
            b.append( hours );
        }

        if( minutes != null && !minutes.isEmpty() ) {
            b.append( ';' );
            b.append( iCalKeys.BYMINUTE );
            b.append( '=' );
            b.append( minutes );
        }

        if( seconds != null && !seconds.isEmpty() ) {
            b.append( ';' );
            b.append( iCalKeys.BYSECOND );
            b.append( '=' );
            b.append( seconds );
        }

        if( position != null && !position.isEmpty() ) {
            b.append( ';' );
            b.append( iCalKeys.BYSETPOS );
            b.append( '=' );
            b.append( position );
        }

        return b.toString();
    }
}

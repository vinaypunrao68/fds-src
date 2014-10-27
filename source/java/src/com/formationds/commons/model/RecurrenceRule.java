/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.commons.model.util.RRIntegerValidator;
import com.formationds.commons.util.Numbers;
import com.formationds.commons.util.WeekDays;
import com.google.gson.annotations.SerializedName;

import java.io.Serializable;
import java.text.ParseException;
import java.util.Date;
import java.util.NoSuchElementException;
import java.util.StringTokenizer;

/**
 * @author ptinius
 */
public class RecurrenceRule
  implements Serializable {
  private static final long serialVersionUID = -6056637443021228929L;

  private static final String MISSING_TOKEN =
    "Missing expected token, last token: '%s'";
  private static final String INVALID_FREQ =
    "Invalid FREQ rule part '%s' in recurrence rule";
  private static final String MUST_CONTAIN =
    "A recurrence rule MUST contain a FREQ rule part.";

  private static final String RRULE_STARTSWITH = "RRULE:";

  static enum iCalKeys {
    FREQ,
    UNTIL,
    COUNT,
    INTERVAL,
    BYSECOND,
    BYMINUTE,
    BYHOUR,
    BYDAY,
    BYMONTHDAY,
    BYYEARDAY,
    BYWEEKNO,
    BYMONTH,
    BYSETPOS,
    WKST
  }

  /**
   * Frequency resolution
   */
  static enum iCalFrequency {
    SECONDLY,
    MINUTELY,
    HOURLY,
    DAILY,
    WEEKLY,
    MONTHLY,
    YEARLY
  }

  /**
   * Week Day
   */
  static enum iCalWeekDay {
    SU( 0 ),
    MO( 1 ),
    TU( 2 ),
    WE( 3 ),
    TH( 4 ),
    FR( 5 ),
    SA( 6 );

    private final int offset;

    iCalWeekDay( final int offset ) {
      this.offset = offset;
    }

    /**
     * @return Returns the offset.
     */
    public final int getOffset() {
      return this.offset;
    }

    protected void validate() {
      iCalWeekDay.valueOf( name() );
    }

    /**
     * @param weekDay the {@link iCalWeekDay}
     *
     * @return Returns {@code true} if equal. Otherwise, {@code false}
     */
    public final boolean equals( final iCalWeekDay weekDay ) {
      return weekDay != null && weekDay.getOffset() == getOffset();
    }

    /**
     * @return the name of this enum constant
     */
    @Override
    public String toString() {
      final StringBuilder b = new StringBuilder();

      if( getOffset() != 0 ) {
        b.append( getOffset() );
      }

      b.append( name() );

      return b.toString();
    }
  }

  @SerializedName( "FREQ" )
  private String frequency;
  @SerializedName( "UNTIL" )
  private Date until;
  @SerializedName( "COUNT" )
  private Integer count = null;
  @SerializedName( "INTERVAL" )
  private Integer interval = null;

  @SerializedName( "BYSECOND" )
  private Numbers<Integer> seconds = new Numbers<>();
  @SerializedName( "BYMINUTE" )
  private Numbers<Integer> minutes = new Numbers<>();
  @SerializedName( "BYHOURS" )
  private Numbers<Integer> hours = new Numbers<>();
  @SerializedName( "BYDAY" )
  private WeekDays<iCalWeekDay> days = new WeekDays<>();
  @SerializedName( "BYMONTHDAY" )
  private Numbers<Integer> monthDays = new Numbers<>();
  @SerializedName( "BYYEAR" )
  private Numbers<Integer> yearDays = new Numbers<>();
  @SerializedName( "BYWEEKNO" )
  private Numbers<Integer> weekNo = new Numbers<>();
  @SerializedName( "BYMONTH" )
  private Numbers<Integer> months = new Numbers<>();
  @SerializedName( "BYSETPOS" )
  private Numbers<Integer> position = new Numbers<>();
  @SerializedName( "WKST" )
  private String weekStartDay;

  /**
   * default constructor
   */
  public RecurrenceRule() {
    seconds.validator( new RRIntegerValidator( 0, 59, false ) );
    minutes.validator( new RRIntegerValidator( 0, 59, false ) );
    hours.validator( new RRIntegerValidator( 0, 23, false ) );
    monthDays.validator( new RRIntegerValidator( 1, 31, true ) );
    months.validator( new RRIntegerValidator( 1, 12, false ) );
    yearDays.validator( new RRIntegerValidator( 1, 366, true ) );
    weekNo.validator( new RRIntegerValidator( 1, 53, true ) );
  }

  /**
   * @param frequency the {@link String} representing the frequency
   * @param until     the {@link java.util.Date} defines a date-time value
   *                  which bounds the recurrence rule in an inclusive manner
   */
  public RecurrenceRule( final String frequency, final Date until ) {
    setFrequency( frequency );
    setUntil( until );
  }

  /**
   * @param frequency the {@link String} representing the frequency
   * @param count     the {@link Integer} defines the number of occurrences at
   *                  which to range-bound the recurrence
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

  /**
   * @return Returns {@link Integer} representing the ical interval
   */
  public Integer getInterval() {
    return interval;
  }

  /**
   * @param interval the {@link Integer} representing the ical interval
   */
  public void setInterval( final Integer interval ) {
    this.interval = interval;
  }

  /**
   * @return Returns {@link Integer} representing the ical count
   */
  public Integer getCount() {
    return count;
  }

  /**
   * @param count the {@link Integer} representing the ical count
   */
  public void setCount( final Integer count ) {
    this.count = count;
  }

  /**
   * @return Returns {@link Date} representing the ical until
   */
  public Date getUntil() {
    return until;
  }

  /**
   * @param until the {@link Date} representing the ical until
   */
  public void setUntil( final Date until ) {
    this.until = until;
  }

  /**
   * @return Returns {@link String} representing the ical week start day
   */
  public String getWeekStartDay() {
    return weekStartDay;
  }

  /**
   * @param weekStartDay the {@link String} representing the ical week start day
   */
  public void setWeekStartDay( final String weekStartDay ) {
    this.weekStartDay = weekStartDay;
  }

  /**
   * @return Returns {@link com.formationds.commons.util.Numbers} of {@link java.lang.Integer}
   */
  public Numbers<Integer> getPosition() {
    return position;
  }

  /**
   * @param position {@link com.formationds.commons.util.Numbers} of {@link java.lang.Integer}
   */
  public void setPosition( final Numbers<Integer> position ) {
    this.position = position;
  }

  /**
   * @return Returns {@link com.formationds.commons.util.Numbers} of {@link java.lang.Integer}
   */
  public Numbers<Integer> getMonths() {
    return months;
  }

  /**
   * @param months {@link com.formationds.commons.util.Numbers} of {@link java.lang.Integer}
   */
  public void setMonths( final Numbers<Integer> months ) {
    this.months = months;
  }

  /**
   * @return Returns {@link com.formationds.commons.util.Numbers} of {@link java.lang.Integer}
   */
  public Numbers<Integer> getWeekNo() {
    return weekNo;
  }

  /**
   * @param weekNo the {@link com.formationds.commons.util.Numbers} of {@link java.lang.Integer}
   */
  public void setWeekNo( final Numbers<Integer> weekNo ) {
    this.weekNo = weekNo;
  }

  /**
   * @return Returns {@link com.formationds.commons.util.Numbers} of {@link java.lang.Integer}
   */
  public Numbers<Integer> getYearDays() {
    return yearDays;
  }

  /**
   * @param yearDays teh {@link com.formationds.commons.util.Numbers} of {@link java.lang.Integer}
   */
  public void setYearDays( final Numbers<Integer> yearDays ) {
    this.yearDays = yearDays;
  }

  /**
   * @return Returns {@link com.formationds.commons.util.Numbers} of {@link java.lang.Integer}
   */
  public Numbers<Integer> getMonthDays() {
    return monthDays;
  }

  /**
   * @param monthDays the {@link com.formationds.commons.util.Numbers} of {@link java.lang.Integer}
   */
  public void setMonthDays( final Numbers<Integer> monthDays ) {
    this.monthDays = monthDays;
  }

  /**
   * @return Returns {@link WeekDays} of {@link iCalWeekDay}
   */
  public WeekDays<iCalWeekDay> getDays() {
    return days;
  }

  /**
   * @param days the {@link WeekDays} of {@link iCalWeekDay}
   */
  public void setDays( final WeekDays<iCalWeekDay> days ) {
    this.days = days;
  }

  /**
   * @return Returns the {@link com.formationds.commons.util.Numbers} of {@link java.lang.Integer}
   */
  public Numbers<Integer> getMinutes() {
    return minutes;
  }

  /**
   * @param minutes the the {@link com.formationds.commons.util.Numbers} of {@link java.lang.Integer}
   */
  public void setMinutes( final Numbers<Integer> minutes ) {
    this.minutes = minutes;
  }

  /**
   * @return Returns the {@link com.formationds.commons.util.Numbers} of {@link java.lang.Integer}
   */
  public Numbers<Integer> getHours() {
    return hours;
  }

  /**
   * @param hours the the {@link com.formationds.commons.util.Numbers} of {@link java.lang.Integer}
   */
  public void setHours( final Numbers<Integer> hours ) {
    this.hours = hours;
  }

  /**
   * @return Returns the {@link com.formationds.commons.util.Numbers} of {@link java.lang.Integer}
   */
  public Numbers<Integer> getSeconds() {
    return seconds;
  }

  /**
   * @param seconds teh the {@link com.formationds.commons.util.Numbers} of {@link java.lang.Integer}
   */
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

      if( iCalKeys.FREQ.equals( iCalKeys.valueOf( token ) ) ) {
        RRule.setFrequency( nextToken( t, token ) );
      } else {
        final iCalKeys key = iCalKeys.valueOf( token );
        if( iCalKeys.UNTIL.equals( iCalKeys.valueOf( token ) ) ) {
          final String untilString = nextToken( t, token );
          RRule.setUntil( ObjectModelHelper.toiCalFormat( untilString ) );
        } else if( iCalKeys.COUNT.equals( key ) ) {
          RRule.setCount( Integer.parseInt( nextToken( t, token ) ) );
        } else if( iCalKeys.INTERVAL.equals( key ) ) {
          RRule.setInterval( Integer.parseInt( nextToken( t, token ) ) );
        } else if( iCalKeys.BYSECOND.equals( key ) ) {
          seconds.add( nextToken( t, token ), "," );
          RRule.setSeconds( seconds );
        } else if( iCalKeys.BYMINUTE.equals( key ) ) {
          minutes.add( nextToken( t, token ), "," );
          RRule.setMinutes( minutes );
        } else if( iCalKeys.BYHOUR.equals( key ) ) {
          hours.add( nextToken( t, token ), "," );
          RRule.setHours( hours );
        } else if( iCalKeys.BYDAY.equals( key ) ) {
          days.add( nextToken( t, token ), "," );
          RRule.setDays( days );
        } else if( iCalKeys.BYMONTHDAY.equals( key ) ) {
          monthDays.add( nextToken( t, token ), "," );
          RRule.setMonthDays( monthDays );
        } else if( iCalKeys.BYYEARDAY.equals( key ) ) {
          yearDays.add( nextToken( t, token ), "," );
          RRule.setYearDays( yearDays );
        } else if( iCalKeys.BYWEEKNO.equals( key ) ) {
          weekNo.add( nextToken( t, token ), "," );
          RRule.setWeekNo( weekNo );
        } else if( iCalKeys.BYMONTH.equals( key ) ) {
          months.add( nextToken( t, token ), "," );
          RRule.setMonths( months );
        } else if( iCalKeys.BYSETPOS.equals( key ) ) {
          position.add( nextToken( t, token ), "," );
          RRule.setPosition( position );
        } else if( iCalKeys.WKST.equals( key ) ) {
          weekStartDay = nextToken( t, token );
          RRule.setWeekStartDay( weekStartDay );
        }
        // SKIP -- assuming its a experimental value.
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
    if( !iCalFrequency.SECONDLY
      .equals( iCalFrequency.valueOf( getFrequency() ) ) &&
      ( !iCalFrequency.MINUTELY
        .equals( iCalFrequency.valueOf( getFrequency() ) ) ) &&
      ( !iCalFrequency.HOURLY
        .equals( iCalFrequency.valueOf( getFrequency() ) ) ) &&
      ( !iCalFrequency.DAILY
        .equals( iCalFrequency.valueOf( getFrequency() ) ) ) &&
      ( !iCalFrequency.WEEKLY
        .equals( iCalFrequency.valueOf( getFrequency() ) ) ) &&
      ( !iCalFrequency.MONTHLY
        .equals( iCalFrequency.valueOf( getFrequency() ) ) ) &&
      ( !iCalFrequency.YEARLY
        .equals( iCalFrequency.valueOf( getFrequency() ) ) ) ) {
      throw new IllegalArgumentException( String.format( INVALID_FREQ,
                                                         getFrequency() ) );
    }
  }

  public final String toString() {
    final StringBuffer b = new StringBuffer();

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
      b.append( until );
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

    if( !months.isEmpty() ) {
      b.append( ';' );
      b.append( iCalKeys.BYMONTH );
      b.append( '=' );
      b.append( months );
    }
    if( !weekNo.isEmpty() ) {
      b.append( ';' );
      b.append( iCalKeys.BYWEEKNO );
      b.append( '=' );
      b.append( weekNo );
    }

    if( !days.isEmpty() ) {
      b.append( ';' );
      b.append( iCalKeys.BYDAY );
      b.append( '=' );
      b.append( days );
    }

    if( !yearDays.isEmpty() ) {
      b.append( ';' );
      b.append( iCalKeys.BYYEARDAY );
      b.append( '=' );
      b.append( yearDays );
    }

    if( !monthDays.isEmpty() ) {
      b.append( ';' );
      b.append( iCalKeys.BYMONTHDAY );
      b.append( '=' );
      b.append( monthDays );
    }

    if( !hours.isEmpty() ) {
      b.append( ';' );
      b.append( iCalKeys.BYHOUR );
      b.append( '=' );
      b.append( hours );
    }

    if( !minutes.isEmpty() ) {
      b.append( ';' );
      b.append( iCalKeys.BYMINUTE );
      b.append( '=' );
      b.append( minutes );
    }

    if( !seconds.isEmpty() ) {
      b.append( ';' );
      b.append( iCalKeys.BYSECOND );
      b.append( '=' );
      b.append( seconds );
    }

    if( !position.isEmpty() ) {
      b.append( ';' );
      b.append( iCalKeys.BYSETPOS );
      b.append( '=' );
      b.append( position );
    }

    return b.toString();
  }
}

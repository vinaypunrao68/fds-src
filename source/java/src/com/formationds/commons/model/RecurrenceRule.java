/*
 * Copyright (C) 2014, All Rights Reserved, by Formation Data Systems, Inc.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;
import com.formationds.commons.model.builder.RecurrenceRuleBuilder;
import com.formationds.commons.model.exception.ParseException;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.commons.model.i18n.ModelResource;
import com.formationds.commons.model.type.iCalFields;
import com.formationds.commons.model.util.iCalValidator;

import javax.xml.bind.annotation.XmlRootElement;
import java.text.MessageFormat;
import java.text.SimpleDateFormat;
import java.util.*;

/**
 * @author ptinius
 */
@XmlRootElement
public class RecurrenceRule
  extends ModelBase {
  private static final long serialVersionUID = 5047799369275124734L;

  private static final SimpleDateFormat UNTIL_DATE =
    new SimpleDateFormat( "yyyyMMdd'T'HHmmssZ" );

  private iCalFields iCalField;
  private Date until;
  private int count = -1;
  private int interval = 0;

  private static final Map<iCalFields, iCalValidator> VALIDATORS =
    new HashMap<>();

  static {
    VALIDATORS.put( iCalFields.BYHOUR, new iCalValidator( 0, 23, false ) );
    VALIDATORS.put( iCalFields.BYMINUTE, new iCalValidator( 0, 59, false ) );
    VALIDATORS.put( iCalFields.BYMONTH, new iCalValidator( 1, 12, false ) );
    VALIDATORS.put( iCalFields.BYMONTHDAY, new iCalValidator( 1, 31, true ) );
    VALIDATORS.put( iCalFields.BYSECOND, new iCalValidator( 0, 59, false ) );
    VALIDATORS.put( iCalFields.BYYEARDAY, new iCalValidator( 1, 366, true ) );
    VALIDATORS.put( iCalFields.BYWEEKNO, new iCalValidator( 1, 53, true ) );
    VALIDATORS.put( iCalFields.BYSECOND, new iCalValidator( 1, 366, true ) );
  }

  /**
   * default constructor
   */
  public RecurrenceRule() {
    super();
  }

  /**
   * @return Returns {@code int} representing the number of times the event
   * recurs
   */
  public int getCount() {
    return count;
  }

  /**
   * @param count the {@code int} representing the number of times the event
   *              recurs
   */
  public void setCount( int count ) {
    this.count = count;
  }

  /**
   * @return Returns {@link String} representing the recurrence frequency (i.e.
   * how often an event recurs)
   */
  public iCalFields getFrequency() {
    return iCalField;
  }

  /**
   * @param iCalField the {@link iCalFields} representing the recurrence field
   *                  (i.e. how often an event recurs)
   */
  public void setFrequency( final iCalFields iCalField ) {
    this.iCalField = iCalField;
    isFrequencyValid();
  }

  /**
   * @return Returns {@link Date} representing the ending date
   */
  public Date getUntil() {
    return until;
  }

  /**
   * @param until the {@link Date} representing the ending date
   */
  public void setUntil( Date until ) {
    this.until = until;
  }

  /**
   * @return Returns {@code int} representing interval between 2 occurrences of
   * the event
   */
  public int getInterval() {
    return interval;
  }

  /**
   * @param interval the {@code int} representing interval between 2
   *                 occurrences of the event
   */
  public void setInterval( int interval ) {
    this.interval = interval;
  }

  /**
   * @throws IllegalArgumentException is frequency is invalid
   */
  public void isFrequencyValid()
    throws IllegalArgumentException {

    if( getFrequency() == null ) {
      throw new IllegalArgumentException(
        ModelResource.getString( "ical.missing.freq" ) );
    }

    if( iCalFields.valueOf( getFrequency().name() ) == null ) {
      throw new IllegalArgumentException(
        MessageFormat.format( ModelResource.getString( "ical.invalid.freq" ),
                              getFrequency() ) );
    }
  }

  /**
   * @param field the {@link iCalFields}
   * @param value the {@code int} value to be validated
   *
   * @return Returns {@code true} if {@code value} is c=valid. Otherwise {@code false}
   */
  protected boolean isValid( final iCalFields field, final int value )
  {
    return VALIDATORS.get( field ).isValid( value );
  }

  /**
   * @param recurrence the {@link String} representing the Recurrence Rule
   *
   * @return Returns {@link com.formationds.commons.model.RecurrenceRule}
   *
   * @throws ParseException if any parse errors are encountered
   */
  public static RecurrenceRule parser( final String recurrence )
    throws ParseException {
    StringTokenizer t = new StringTokenizer( recurrence, ";=" );

    RecurrenceRuleBuilder builder = new RecurrenceRuleBuilder();
    while( t.hasMoreTokens() ) {
      String token = t.nextToken();

      if( iCalFields.FREQ.name()
                         .equals( token ) ) {
        builder = builder.withFrequency( next( t, token ) );
      } else if( iCalFields.COUNT.name().equals( token ) ) {
        try {
          builder = builder.withCount( Integer.valueOf( next( t, token ) ) );
        } catch( NumberFormatException e ) {
          throw new ParseException( e.getMessage(), token );
        }
      } else if( iCalFields.INTERVAL.name().equals( token ) ) {
        try {
          builder = builder.withInterval( Integer.valueOf( next( t, token ) ) );
        } catch( NumberFormatException e ) {
          throw new ParseException( token );
        }
      } else if( iCalFields.UNTIL.name().equals( token ) ) {
        try {
          builder = builder.withUntil( ObjectModelHelper.toiCalFormat( next( t, token ) ) );
        } catch( java.text.ParseException e ) {
          throw new ParseException( token );
        }
      }
      // TODO finish implementing the rest of the iCal RRULE syntax
    }

    return builder.build();
  }

  private static String next( final StringTokenizer t,
                              final String lastToken ) {
    try {
      return t.nextToken();
    } catch (NoSuchElementException ignored ) {
    }

    throw new IllegalArgumentException(
      String.format( ModelResource.getString( "ical.missing.token" ),
                     lastToken ));
  }

// TODO finish filling in the rest of the iCal Recurrence Rule properties
  /*
    The value type is defined by the following
     notation:

     recur      = "FREQ"=freq *(

     ; either UNTIL or COUNT may appear in a 'recur',
     ; but UNTIL and COUNT MUST NOT occur in the same 'recur'

     ( ";" "UNTIL" "=" enddate ) /
     ( ";" "COUNT" "=" 1*DIGIT ) /

     ; the rest of these keywords are optional,
     ; but MUST NOT occur more than once

     ( ";" "INTERVAL" "=" 1*DIGIT )          /
     ( ";" "BYSECOND" "=" byseclist )        /
     ( ";" "BYMINUTE" "=" byminlist )        /
     ( ";" "BYHOUR" "=" byhrlist )           /
     ( ";" "BYDAY" "=" bywdaylist )          /
     ( ";" "BYMONTHDAY" "=" bymodaylist )    /
     ( ";" "BYYEARDAY" "=" byyrdaylist )     /
     ( ";" "BYWEEKNO" "=" bywknolist )       /
     ( ";" "BYMONTH" "=" bymolist )          /
     ( ";" "BYSETPOS" "=" bysplist )         /
     ( ";" "WKST" "=" weekday )              /
     ( ";" x-name "=" text )
     )

     freq       = "SECONDLY" / "MINUTELY" / "HOURLY" / "DAILY"
     / "WEEKLY" / "MONTHLY" / "YEARLY"

     enddate    = date
     enddate    =/ date-time            ;An UTC value

     byseclist  = seconds / ( seconds *("," seconds) )

     seconds    = 1DIGIT / 2DIGIT       ;0 to 59

     byminlist  = minutes / ( minutes *("," minutes) )

     minutes    = 1DIGIT / 2DIGIT       ;0 to 59

     byhrlist   = hour / ( hour *("," hour) )

     hour       = 1DIGIT / 2DIGIT       ;0 to 23

     bywdaylist = weekdaynum / ( weekdaynum *("," weekdaynum) )

     weekdaynum = [([plus] ordwk / minus ordwk)] weekday

     plus       = "+"

     minus      = "-"

     ordwk      = 1DIGIT / 2DIGIT       ;1 to 53

     weekday    = "SU" / "MO" / "TU" / "WE" / "TH" / "FR" / "SA"
     ;Corresponding to SUNDAY, MONDAY, TUESDAY, WEDNESDAY, THURSDAY,
     ;FRIDAY, SATURDAY and SUNDAY days of the week.

     bymodaylist = monthdaynum / ( monthdaynum *("," monthdaynum) )

     monthdaynum = ([plus] ordmoday) / (minus ordmoday)

     ordmoday   = 1DIGIT / 2DIGIT       ;1 to 31

     byyrdaylist = yeardaynum / ( yeardaynum *("," yeardaynum) )

     yeardaynum = ([plus] ordyrday) / (minus ordyrday)

     ordyrday   = 1DIGIT / 2DIGIT / 3DIGIT      ;1 to 366

     bywknolist = weeknum / ( weeknum *("," weeknum) )
     weeknum    = ([plus] ordwk) / (minus ordwk)

     bymolist   = monthnum / ( monthnum *("," monthnum) )

     monthnum   = 1DIGIT / 2DIGIT       ;1 to 12

     bysplist   = setposday / ( setposday *("," setposday) )

     setposday  = yeardaynum
 */

  /**
   * @return Returns {@link String} representing this object
   */
  @Override
  public String toString() {
    final StringBuilder sb = new StringBuilder( );

    sb.append( iCalFields.FREQ.name() )
      .append( "=" )
      .append( getFrequency() )
      .append( ";" );

    if( getUntil() != null )
    {
      sb.append( iCalFields.UNTIL.name() )
        .append( "=" )
        .append( UNTIL_DATE.format( getUntil() ) )
        .append( ";" );
    }

    if( getInterval() > 0 )
    {
      sb.append( iCalFields.INTERVAL.name() )
        .append( "=" )
        .append( getInterval() )
        .append( ";" );
    }

    if( getCount() > 0 )
    {
      sb.append( iCalFields.COUNT.name() )
        .append( "=" )
        .append( getCount() )
        .append( ";" );
    }

    return sb.substring( 0, sb.length() - 1 ).trim();
  }
}

/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.builder;

import com.formationds.commons.model.RecurrenceRule;
import com.formationds.commons.model.type.iCalFrequency;
import com.formationds.commons.model.type.iCalWeekDays;
import com.formationds.commons.util.Numbers;
import com.formationds.commons.util.WeekDays;

import java.util.Date;

/**
 * @author ptinius
 */
public class RecurrenceRuleBuilder {
    private final iCalFrequency frequency;

    private Date until;
    private Integer count = null;
    private Integer interval = null;
    private Numbers<Integer> seconds = null;
    private Numbers<Integer> minutes = null;
    private Numbers<Integer> hours = null;
    private Numbers<Integer> monthDays = null;
    private Numbers<Integer> yearDays = null;
    private Numbers<Integer> weekNo = null;
    private Numbers<Integer> months = null;
    private Numbers<Integer> position = null;
    private String weekStartDay;
    private WeekDays<iCalWeekDays> days = null;

    public RecurrenceRuleBuilder( final iCalFrequency frequency ) {
        this.frequency = frequency;
    }

    public RecurrenceRuleBuilder withUntil( Date until ) {
        this.until = until;
        return this;
    }

    public RecurrenceRuleBuilder withCount( Integer count ) {
        this.count = count;
        return this;
    }

    public RecurrenceRuleBuilder withInterval( Integer interval ) {
        this.interval = interval;
        return this;
    }

    public RecurrenceRuleBuilder bySecond( Numbers<Integer> seconds ) {
        this.seconds = seconds;
        return this;
    }

    public RecurrenceRuleBuilder byMinute( Numbers<Integer> minutes ) {
        this.minutes = minutes;
        return this;
    }

    public RecurrenceRuleBuilder byHour( Numbers<Integer> hours ) {
        this.hours = hours;
        return this;
    }

    public RecurrenceRuleBuilder byMonthDay( Numbers<Integer> monthDays ) {
        this.monthDays = monthDays;
        return this;
    }

    public RecurrenceRuleBuilder byYearDay( Numbers<Integer> yearDays ) {
        this.yearDays = yearDays;
        return this;
    }

    public RecurrenceRuleBuilder byWeekNo( Numbers<Integer> weekNo ) {
        this.weekNo = weekNo;
        return this;
    }

    public RecurrenceRuleBuilder byMonth( Numbers<Integer> months ) {
        this.months = months;
        return this;
    }

    public RecurrenceRuleBuilder byPosition( Numbers<Integer> position ) {
        this.position = position;
        return this;
    }

    public RecurrenceRuleBuilder byWeekStartDay( String weekStartDay ) {
        this.weekStartDay = weekStartDay;
        return this;
    }

    public RecurrenceRuleBuilder byDay( WeekDays<iCalWeekDays> days ) {
        this.days = days;
        return this;
    }

    public RecurrenceRule build() {
        RecurrenceRule recurrenceRule = new RecurrenceRule();

        recurrenceRule.setFrequency( frequency.name() );

        if( until != null ) {
            recurrenceRule.setUntil( until );
        }
        if( count != null ) {
            recurrenceRule.setCount( count );
        }
        if( interval != null ) {
            recurrenceRule.setInterval( interval );
        }
        if( seconds != null ) {
            recurrenceRule.setSeconds( seconds );
        }

        if( minutes != null ) {
            recurrenceRule.setMinutes( minutes );
        }

        if( hours != null ) {
            recurrenceRule.setHours( hours );
        }

        if( monthDays != null ) {
            recurrenceRule.setMonthDays( monthDays );
        }
        if( yearDays != null ) {
            recurrenceRule.setYearDays( yearDays );
        }

        if( weekNo != null ) {
            recurrenceRule.setWeekNo( weekNo );
        }

        if( months != null ) {
            recurrenceRule.setMonths( months );
        }

        if( position != null ) {
            recurrenceRule.setPosition( position );
        }

        if( weekStartDay != null ) {
            recurrenceRule.setWeekStartDay( weekStartDay );
        }

        if( days != null ) {
            recurrenceRule.setDays( days );
        }

        return recurrenceRule;
    }
}

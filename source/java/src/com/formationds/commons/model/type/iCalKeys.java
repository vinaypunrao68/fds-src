/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.commons.model.type;

import com.formationds.commons.model.RecurrenceRule;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.commons.model.util.RRIntegerValidator;
import com.formationds.commons.util.Numbers;
import com.formationds.commons.util.WeekDays;

import java.text.ParseException;
import java.util.Collection;
import java.util.Date;
import java.util.Optional;

/**
 * @author ptinius
 */
public enum iCalKeys {

    FREQ {
        public void setRecurrenceRule(RecurrenceRule rule, String n) throws ParseException {
            rule.setFrequency(n);
        }

        public Optional<String> formatKV( RecurrenceRule rule) {
            String s = rule.getFrequency();
            return formatKV(this, s);
        }
    },

    UNTIL {
        public void setRecurrenceRule(RecurrenceRule rule, String n) throws ParseException {
            rule.setUntil(ObjectModelHelper.toiCalFormat(n));
        }

        public Optional<String> formatKV(RecurrenceRule rule) {
            Date d = rule.getUntil();
            return formatKV(this, d);
        }
    },

    COUNT {
        public void setRecurrenceRule(RecurrenceRule rule, String n) throws ParseException {
            rule.setCount(Integer.parseInt(n));
        }

        public Optional<String> formatKV(RecurrenceRule rule) {
            Integer i = rule.getCount();
            return formatKV(this, i);
        }
    },

    INTERVAL {
        public void setRecurrenceRule(RecurrenceRule rule, String n) throws ParseException {
            rule.setInterval(Integer.parseInt(n));
        }

        public Optional<String> formatKV(RecurrenceRule rule) {
            Integer i = rule.getInterval();
            return formatKV(this, i);
        }
    },

    BYSECOND {
        public void setRecurrenceRule(RecurrenceRule rule, String n) throws ParseException {
            Numbers<Integer> seconds = rule.getSeconds();
            if (seconds == null) {
                seconds = new Numbers<>();
                seconds.validator(new RRIntegerValidator(0, 59, false));
                rule.setSeconds(seconds);
            }
            seconds.add(n, ",");
        }

        public Optional<String> formatKV(RecurrenceRule rule) {
            Numbers<?> s = rule.getSeconds();
            return formatKV(this, s);
        }
    },

    BYMINUTE {
        public void setRecurrenceRule(RecurrenceRule rule, String n) throws ParseException {
            Numbers<Integer> minutes = rule.getMinutes();
            if (minutes == null) {
                minutes = new Numbers<>();
                minutes.validator(new RRIntegerValidator(0, 59, false));
                rule.setMinutes(minutes);
            }
            minutes.add(n, ",");
        }

        public Optional<String> formatKV(RecurrenceRule rule) {
            Numbers<?> s = rule.getMinutes();
            return formatKV(this, s);
        }
    },

    BYHOUR {
        public void setRecurrenceRule(RecurrenceRule rule, String n) throws ParseException {
            Numbers<Integer> hours = rule.getHours();
            if (hours == null) {
                hours = new Numbers<>();
                hours.validator(new RRIntegerValidator(0, 23, false));
                rule.setHours(hours);
            }
            hours.add(n, ",");
        }

        public Optional<String> formatKV(RecurrenceRule rule) {
            Numbers<?> s = rule.getHours();
            return formatKV(this, s);
        }
    },

    BYDAY {
        public void setRecurrenceRule(RecurrenceRule rule, String n) throws ParseException {
            WeekDays<iCalWeekDays> days = rule.getDays();
            if (days == null) {
                days = new WeekDays<>();
                rule.setDays(days);
            }
            days.add(n, ",");
        }

        public Optional<String> formatKV(RecurrenceRule rule) {
            WeekDays<iCalWeekDays> s = rule.getDays();
            return formatKV(this, s);
        }
    },

    BYMONTHDAY {
        public void setRecurrenceRule(RecurrenceRule rule, String n) throws ParseException {
            Numbers<Integer> monthDays = rule.getMonthDays();
            if (monthDays == null) {
                monthDays = new Numbers<>();
                monthDays.validator(new RRIntegerValidator(1, 31, true));
                rule.setMonthDays(monthDays);
            }
            monthDays.add(n, ",");
        }

        public Optional<String> formatKV(RecurrenceRule rule) {
            Numbers<?> s = rule.getMonthDays();
            return formatKV(this, s);
        }
    },

    BYYEARDAY {
        public void setRecurrenceRule(RecurrenceRule rule, String n) throws ParseException {
            Numbers<Integer> yearDays = rule.getYearDays();
            if (yearDays == null) {
                yearDays = new Numbers<>();
                yearDays.validator(new RRIntegerValidator(1, 366, true));
                rule.setYearDays(yearDays);
            }
            yearDays.add(n, ",");
        }

        public Optional<String> formatKV(RecurrenceRule rule) {
            Numbers<?> s = rule.getYearDays();
            return formatKV(this, s);
        }
    },

    BYWEEKNO {
        public void setRecurrenceRule(RecurrenceRule rule, String n) throws ParseException {
            Numbers<Integer> weekNo = rule.getWeekNo();
            if (weekNo == null) {
                weekNo = new Numbers<>();
                weekNo.validator(new RRIntegerValidator(1, 53, true));
                rule.setWeekNo(weekNo);
            }
            weekNo.add(n, ",");
        }

        public Optional<String> formatKV(RecurrenceRule rule) {
            Numbers<?> s = rule.getWeekNo();
            return formatKV(this, s);
        }
    },

    BYMONTH {
        public void setRecurrenceRule(RecurrenceRule rule, String n) throws ParseException {
            Numbers<Integer> months = rule.getMonths();
            if (months == null) {
                months = new Numbers<>();
                months.validator(new RRIntegerValidator(1, 12, false));
                rule.setMonths(months);
            }
            months.add(n, ",");
        }

        public Optional<String> formatKV(RecurrenceRule rule) {
            Numbers<?> s = rule.getMonths();
            return formatKV(this, s);
        }
    },

    BYSETPOS {
        public void setRecurrenceRule(RecurrenceRule rule, String n) throws ParseException {
            Numbers<Integer> position = rule.getPosition();
            if (position == null) {
                position = new Numbers<>();
                rule.setPosition(position);
            }
            position.add(n, ",");
        }

        public Optional<String> formatKV(RecurrenceRule rule) {
            return formatKV(this, rule.getPosition());
        }
    },

    WKST {
        public void setRecurrenceRule(RecurrenceRule rule, String n) throws ParseException {
            rule.setWeekStartDay(n);
        }

        public Optional<String> formatKV(RecurrenceRule rule) {
            String s = rule.getWeekStartDay();
            return formatKV(this, s);
        }
    };

    /**
     * Set the recurrence rule field based on the specified token.
     *
     * @param rule the recurrence rule to set the field in
     * @param n the token to set the rule base don
     *
     * @throws ParseException
     */
    abstract public void setRecurrenceRule(RecurrenceRule rule, String n) throws ParseException;

    /**
     * Format the key-value pair using the RecurenceRule value for this key.
     * <p>
     * The value is only included if it is non-null and non-empty.
     *
     * @param rule the rule to format
     *
     * @return the the formatted key-value pair or an empty optional if null or empty
     */
    abstract public Optional<String> formatKV(RecurrenceRule rule);

    /**
     * Format the key-value pair if the value is not null or empty.
     *
     * @param key the key
     * @param val the value
     *
     * @return the the formatted key-value pair or an empty optional if null or empty
     */
    protected static Optional<String> formatKV(iCalKeys key, Collection<?> val) {
        return formatKV(key, (val != null && !val.isEmpty() ? val.toString() : null));
    }

    /**
     * Format the key-value pair if the value is not null.
     *
     * @param key the key
     * @param val the Integer value
     *
     * @return the the formatted key-value pair or an empty optional if null or empty
     */
    protected static Optional<String> formatKV(iCalKeys key, Integer val) {
        return formatKV(key, (val != null ? Integer.toString(val) : null));
    }

    /**
     * Format the key-value pair and append to the StringBuilder if the date value is not null.
     *
     * @param key the key
     * @param val the date value
     *
     * @return the the formatted key-value pair or an empty optional if null or empty
     */
    protected static Optional<String> formatKV(iCalKeys key, Date val) {
        return formatKV(key, (val != null ? ObjectModelHelper.isoDateTimeUTC(val) : null));
    }

    /**
     * Format the key-value pair if the value is not null.
     *
     * @param key the key
     * @param val the value
     *
     * @return the the formatted key-value pair or an empty optional if null or empty
     */
    protected static Optional<String> formatKV(iCalKeys key, String val) {
        if (val != null && !val.isEmpty()) {
            return Optional.of(String.format("%s=%s", key.name(), val));
        }
        return Optional.empty();
    }
}

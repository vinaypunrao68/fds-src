/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.builder;

import com.formationds.commons.model.RecurrenceRule;

import java.util.Date;

/**
 * @author ptinius
 */
public class RecurrenceRuleBuilder {
  private String frequency;
  private Date until = null;
  private Integer count;
  private Integer interval;

  /**
   * default constructor
   */
  public RecurrenceRuleBuilder() {
  }

  /**
   * @param frequency the {@link String} representing the frequency
   *
   * @return Returns {link RecurrenceRuleBuilder}
   */
  public RecurrenceRuleBuilder withFrequency( final String frequency ) {
    this.frequency = frequency;
    return this;
  }

  /**
   * @param until the {@link String} representing the until
   *
   * @return Returns {link RecurrenceRuleBuilder}
   */
  public RecurrenceRuleBuilder withUntil( final Date until ) {
    this.until = until;
    return this;
  }

  /**
   * @param count the {@code int} representing the occurrence count
   *
   * @return Returns {link RecurrenceRuleBuilder}
   */
  public RecurrenceRuleBuilder withCount( final int count ) {
    this.count = count;
    return this;
  }

  /**
   * @param interval the {@code interval} representing the interval
   *
   * @return Returns {link RecurrenceRuleBuilder}
   */
  public RecurrenceRuleBuilder withInterval( final int interval ) {
    this.interval = interval;
    return this;
  }

  /**
   * @return Returns {link RecurrenceRule}
   */
  public RecurrenceRule build() {
    RecurrenceRule recurrenceRule = new RecurrenceRule();
    recurrenceRule.setFrequency( frequency );
    if( until != null )
    {
      recurrenceRule.setUntil( until );
    }

    if( count != null ) {
      recurrenceRule.setCount( count );
    }

    if( interval != null ) {
      recurrenceRule.setInterval( interval );
    }

    return recurrenceRule;
  }
}

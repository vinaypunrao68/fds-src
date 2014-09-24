/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.builder;

import com.formationds.commons.model.RecurrenceRule;
import com.formationds.commons.model.type.iCalFields;

import java.util.Date;

/**
 * @author ptinius
 */
public class RecurrenceRuleBuilder {
  private iCalFields frequency;
  private Date until = null;
  private int count = -1;
  private int interval = 0;

  /**
   * defULT CONSRTUCTOR
   */
  public RecurrenceRuleBuilder() {
  }

  /**
   * @param frequency the {@link String} representing the frequency
   *
   * @return Returns {link RecurrenceRuleBuilder}
   */
  public RecurrenceRuleBuilder withFrequency( final String frequency ) {
    return withFrequency( iCalFields.valueOf( frequency ) );
  }

  /**
   * @param frequency the {@link iCalFields} representing the frequency
   *
   * @return Returns {link RecurrenceRuleBuilder}
   */
  public RecurrenceRuleBuilder withFrequency( final iCalFields frequency ) {
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

    if( count > 0 ) {
      recurrenceRule.setCount( count );
    }

    if( interval > 0 ) {
      recurrenceRule.setInterval( interval );
    }

    return recurrenceRule;
  }
}

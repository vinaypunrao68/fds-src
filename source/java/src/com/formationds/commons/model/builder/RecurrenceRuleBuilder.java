/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.builder;

import com.formationds.commons.model.RecurrenceRule;
import com.google.gson.annotations.SerializedName;

import java.util.Date;

/**
 * @author ptinius
 */
public class RecurrenceRuleBuilder {
  @SerializedName( "FREQ" )
  private String frequency;
  @SerializedName( "UNTIL" )
  private Date UNTIL;
  @SerializedName( "COUNT" )
  private Integer COUNT;
  @SerializedName( "INTERVAL" )
  private Integer INTERVAL;

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
   * @param UNTIL the {@link String} representing the until
   *
   * @return Returns {link RecurrenceRuleBuilder}
   */
  public RecurrenceRuleBuilder withUntil( final Date UNTIL ) {
    this.UNTIL = UNTIL;
    return this;
  }

  /**
   * @param COUNT the {@code int} representing the occurrence count
   *
   * @return Returns {link RecurrenceRuleBuilder}
   */
  public RecurrenceRuleBuilder withCount( final int COUNT ) {
    this.COUNT = COUNT;
    return this;
  }

  /**
   * @param INTERVAL the {@code interval} representing the interval
   *
   * @return Returns {link RecurrenceRuleBuilder}
   */
  public RecurrenceRuleBuilder withInterval( final int INTERVAL ) {
    this.INTERVAL = INTERVAL;
    return this;
  }

  /**
   * @return Returns {link RecurrenceRule}
   */
  public RecurrenceRule build() {
    RecurrenceRule recurrenceRule = new RecurrenceRule();
    recurrenceRule.setFrequency( frequency );
    if( UNTIL != null )
    {
      recurrenceRule.setUntil( UNTIL );
    }

    if( COUNT != null ) {
      recurrenceRule.setCount( COUNT );
    }

    if( INTERVAL != null ) {
      recurrenceRule.setInterval( INTERVAL );
    }

    return recurrenceRule;
  }
}

/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 *
 * This software is furnished under a license and may be used and copied only
 * in  accordance  with  the  terms  of such  license and with the inclusion
 * of the above copyright notice. This software or  any  other copies thereof
 * may not be provided or otherwise made available to any other person.
 * No title to and ownership of  the  software  is  hereby transferred.
 *
 * The information in this software is subject to change without  notice
 * and  should  not be  construed  as  a commitment by Formation Data Systems.
 *
 * Formation Data Systems assumes no responsibility for the use or  reliability
 * of its software on equipment which is not supplied by Formation Date Systems.
 */

package com.formationds.commons.model.builder;

import com.formationds.commons.model.RecurrenceRule;

import java.util.Date;

/**
 * @author ptinius
 */
public class RecurrenceRuleBuilder {
  private String frequency;
  private Date until;
  private int count = -1;
  private int interval = 0;

  private RecurrenceRuleBuilder() {
  }

  public static RecurrenceRuleBuilder aRecurrenceRule() {
    return new RecurrenceRuleBuilder();
  }

  /**
   * @param frequency
   *
   * @return Returns {link RecurrenceRuleBuilder}
   */
  public RecurrenceRuleBuilder withFrequency( final String frequency ) {
    this.frequency = frequency;
    return this;
  }

  /**
   * @param until
   *
   * @return Returns {link RecurrenceRuleBuilder}
   */
  public RecurrenceRuleBuilder withUntil( final Date until ) {
    this.until = until;
    return this;
  }

  /**
   * @param count
   *
   * @return Returns {link RecurrenceRuleBuilder}
   */
  public RecurrenceRuleBuilder withCount( final int count ) {
    this.count = count;
    return this;
  }

  /**
   * @param interval
   *
   * @return Returns {link RecurrenceRuleBuilder}
   */
  public RecurrenceRuleBuilder withInterval( final int interval ) {
    this.interval = interval;
    return this;
  }

  /**
   * @return Returns {link RecurrenceRuleBuilder}
   */
  public RecurrenceRuleBuilder but() {
    return aRecurrenceRule().withFrequency( frequency )
                            .withUntil( until )
                            .withCount( count )
                            .withInterval( interval );
  }

  /**
   * @return Returns {link RecurrenceRule}
   */
  public RecurrenceRule build() {
    RecurrenceRule recurrenceRule = new RecurrenceRule();
    recurrenceRule.setFrequency( frequency );
    recurrenceRule.setUntil( until );
    recurrenceRule.setCount( count );
    recurrenceRule.setInterval( interval );
    return recurrenceRule;
  }
}

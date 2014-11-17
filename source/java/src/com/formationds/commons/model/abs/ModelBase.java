/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.abs;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.commons.model.intf.Tagable;
import com.formationds.commons.model.type.ManagerType;
import com.formationds.commons.model.type.NodeState;
import org.apache.commons.lang3.builder.ToStringBuilder;

import javax.xml.bind.annotation.XmlRootElement;

/**
 * @author ptinius
 */
@XmlRootElement
@JsonInclude(JsonInclude.Include.NON_EMPTY)
public abstract class ModelBase
  implements Tagable {
  private static final long serialVersionUID = -7645839798777744738L;

  protected static final Long SLA_MIN = 0L;         // unlimited
  protected static final Long SLA_MAX = 100L;       // percentage
  protected static final Long CAPACITY_MIN = 0L;    // percentage
  protected static final Long CAPACITY_MAX = 100L;  // percentage
  protected static final Long PRIORITY_MIN = 1L;
  protected static final Long PRIORITY_MAX = 10L;

  /**
   * @param field the {@link String} representing the field checking
   *
   * @return Returns {@code true} if {@code field} is set
   */
  protected boolean isSet( final String field ) {
    return ( field != null );
  }

  /**
   * @param field the {@code int} representing the field checking
   *
   * @return Returns {@code true} if {@code field} is set
   */
  protected boolean isSet( final int field ) {
    return field != -1;
  }

  /**
   * @param field the {@code long} representing the field checking
   *
   * @return Returns {@code true} if {@code field} is set
   */
  protected boolean isSet( final long field ) {
    return field != -1L;
  }

  /**
   * @param field the {@link ManagerType} representing the field checking
   *
   * @return Returns {@code true} if {@code field} is set
   */
  protected boolean isSet( final ManagerType field ) {
    return field != null;
  }

  /**
   * @param field the {@link NodeState} representing the field checking
   *
   * @return Returns {@code true} if {@code field} is set
   */
  protected boolean isSet( final NodeState field ) {
    return field != null;
  }

  /**
   * @return Returns {@link String} representing this object as JSON
   */
  public String toJSON() {
    return ObjectModelHelper.toJSON( this );
  }

  /**
   * @param json the {@link String} representing the JSON
   *
   * @return Returns {@link T} representing the {@code json} as a Java {@link
   * Object}
   */
  public <T> T toObject( final String json ) {
    return ObjectModelHelper.toObject( json, this.getClass() );
  }

  /**
   * @return Returns a {@link String} representing this object
   */
  @Override
  public String toString() {
    return ToStringBuilder.reflectionToString( this );
  }
}

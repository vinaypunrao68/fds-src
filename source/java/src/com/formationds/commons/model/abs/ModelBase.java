/*
 * Copyright (C) 2014, All Rights Reserved, by Formation Data Systems, Inc.
 *
 *  This software is furnished under a license and may be used and copied only
 *  in  accordance  with  the  terms  of such  license and with the inclusion
 *  of the above copyright notice. This software or  any  other copies thereof
 *  may not be provided or otherwise made available to any other person.
 *  No title to and ownership of  the  software  is  hereby transferred.
 *
 *  The information in this software is subject to change without  notice
 *  and  should  not be  construed  as  a commitment by Formation Data Systems.
 *
 *  Formation Data Systems assumes no responsibility for the use or  reliability
 *  of its software on equipment which is not supplied by Formation Date Systems.
 */

package com.formationds.commons.model.abs;

import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.commons.model.intr.Tagable;
import com.formationds.commons.model.type.ManagerType;
import com.formationds.commons.model.type.NodeState;
import org.apache.commons.lang3.builder.ToStringBuilder;

import javax.xml.bind.annotation.XmlRootElement;

/**
 * @author ptinius
 */
@XmlRootElement
public abstract class ModelBase
  implements Tagable {
  private static final long serialVersionUID = -7645839798777744738L;

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
   * @return Returns a {@link String} representing this object
   */
  @Override
  public String toString() {
    return ToStringBuilder.reflectionToString( this );
  }
}

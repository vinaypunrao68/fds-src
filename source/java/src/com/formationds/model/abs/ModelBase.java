/*
 * Copyright (C) 2014, All Rights Reserved, by Formation Data Systems, Inc.
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

package com.formationds.model.abs;

import com.formationds.model.intr.Tagable;

import javax.xml.bind.annotation.XmlRootElement;

/**
 * @author ptinius
 */
@XmlRootElement
public abstract class ModelBase
  implements Tagable
{
  private static final long serialVersionUID = -1;

  // TODO add the base object shared methods

  /**
   * @param attribute the object attribute to set if its set
   *
   * @return Returns {@code true} if {@code attribute} is set
   */
  protected boolean isSet( final String attribute )
  {
    return ( attribute != null );
  }

  /**
   * @param attribute the object attribute to set if its set
   *
   * @return Returns {@code true} if {@code attribute} is set
   */
  protected boolean isSet( final int attribute )
  {
    return attribute != -1;
  }

  /**
   * @param attribute the object attribute to set if its set
   *
   * @return Returns {@code true} if {@code attribute} is set
   */
  protected boolean isSet( final long attribute )
  {
    return attribute != -1L;
  }
}

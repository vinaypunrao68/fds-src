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

package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;
import com.formationds.util.SizeUnit;

import javax.xml.bind.annotation.XmlRootElement;

/**
 * @author ptinius
 */
@XmlRootElement
public class Usage
  extends ModelBase
{
  private static final long serialVersionUID = -2938435229361709388L;

  private SizeUnit unit;
  private String size;

  /**
   * default package level constructor
   */
  public Usage()
  {
    super();
  }

  /**
   * Property bound setter for {@code size}.
   */
  public void setSize( String size )
  {;
    this.size = size;
  }

  /**
   * Property bound setter for {@code size}.
   */
  public String getSize()
  {
    return size;
  }

  /**
   * Property bound setter for {@code unit}.
   */
  public void setUnit( SizeUnit unit )
  {
    this.unit = unit;
  }

  /**
   * Property bound setter for {@code unit}.
   */
  public SizeUnit getUnit()
  {
    return unit;
  }
}

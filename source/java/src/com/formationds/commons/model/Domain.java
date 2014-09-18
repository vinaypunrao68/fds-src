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

import javax.xml.bind.annotation.XmlRootElement;

/**
 * @author ptinius
 */
@XmlRootElement
public class Domain
  extends ModelBase
{
  private static final long serialVersionUID = -1;

  private int id;
  private String site;
  private String domain;

  /**
   * default package level constructor
   */
  Domain()
  {
    super();
  }

  /**
   * @return Returns {@code int} representing the domain id
   */
  public int getId()
  {
    return id;
  }

  /**
   * @param id the {@code int} representing the domain id
   */
  public void setId( final int id )
  {
    this.id = id;
  }

  /**

   * @return Returns the {@link String} representing the site's name
   */
  public String getSite()
  {
    return site;
  }

  /**
   * @param site the {@link String} representing the site's name
   */
  public void setSite( final String site )
  {
    this.site = site;
  }

  /**
   * @return Returns {@link String} representing the domain's name
   */
  public String getDomain()
  {
    return domain;
  }

  /**
   * @param domain the {@link String} representing the domain's name
   */
  public void setDomain( final String domain )
  {
    this.domain = domain;
  }
}

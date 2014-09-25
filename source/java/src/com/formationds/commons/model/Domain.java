/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
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
  private static final long serialVersionUID = 2054093416617447166L;

  private int id;
  private String site;
  private String domain;

  /**
   * default package level constructor
   */
  public Domain()
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

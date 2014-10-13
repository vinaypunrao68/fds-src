/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;

/**
 * @author ptinius
 */
public class Domain
  extends ModelBase {
  private static final long serialVersionUID = 2054093416617447166L;

  private Integer id;
  private String site;
  private String domain;

  /**
   * default package level constructor
   */
  public Domain() {
    super();
  }

  /**
   * @return Returns {@link Integer} representing the domain id
   */
  public Integer getId() {
    return id;
  }

  /**
   * @param id the {@link Integer} representing the domain id
   */
  public void setId( final Integer id ) {
    this.id = id;
  }

  /**
   * @return Returns the {@link String} representing the site's name
   */
  public String getSite() {
    return site;
  }

  /**
   * @param site the {@link String} representing the site's name
   */
  public void setSite( final String site ) {
    this.site = site;
  }

  /**
   * @return Returns {@link String} representing the domain's name
   */
  public String getDomain() {
    return domain;
  }

  /**
   * @param domain the {@link String} representing the domain's name
   */
  public void setDomain( final String domain ) {
    this.domain = domain;
  }
}

/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.builder;

import com.formationds.commons.model.Domain;

/**
 * @author ptinius
 */
public class DomainBuilder {
  private int id;
  private String site;
  private String domainName;

  /**
   * default constructor
   */
  public DomainBuilder() {
  }

  /**
   * @param id the {@link int} representing the domain's id
   *
   * @return Returns the {@link DomainBuilder}
   */
  public DomainBuilder withId( int id ) {
    this.id = id;
    return this;
  }

  /**
   * @param site the {@link String} representing the site
   *
   * @return Returns the {@link DomainBuilder}
   */
  public DomainBuilder withSite( String site ) {
    this.site = site;
    return this;
  }

  /**
   * @param domain the {@link String} representing the domain name
   *
   * @return Returns the {@link DomainBuilder}
   */
  public DomainBuilder withDomain( String domain ) {
    this.domainName = domain;
    return this;
  }

  /**
   * @return Returns {@link Domain}
   */
  public Domain build() {
    Domain domain = new Domain();
    domain.setId( id );
    domain.setSite( site );
    domain.setDomain( domainName );
    return domain;
  }
}

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

import com.formationds.commons.model.Domain;

/**
 * @author ptinius
 */
public class DomainBuilder {
  private int id;
  private String site;
  private String domainName;

  /**
   * static utility method constructor
   */
  private DomainBuilder() {
  }

  /**
   * @return Returns the {@link DomainBuilder}
   */
  public static DomainBuilder aDomain() {
    return new DomainBuilder();
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
   * @return Returns the {@link DomainBuilder}
   */
  public DomainBuilder but() {
    return aDomain().withId( id )
                    .withSite( site )
                    .withDomain( domainName );
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

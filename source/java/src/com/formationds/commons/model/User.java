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
import com.formationds.commons.model.type.IdentityType;

import javax.xml.bind.annotation.XmlRootElement;
import javax.xml.bind.annotation.XmlTransient;

/**
 * @author ptinius
 */
@XmlRootElement
public class User
  extends ModelBase
{
  private static final long serialVersionUID = -2139739643610025641L;

  private long id;
  private IdentityType identifier;

  @XmlTransient
  private String passwordHash;
  @XmlTransient
  private String secret;

  /**
   * default constructor
   */
  public User()
  {
    super();
  }

  /**
   * @return Returns the user's id
   */
  public long getId()
  {
    return id;
  }

  /**
   * @param id the user's id
   */
  public void setId( final long id )
  {
    this.id = id;
  }

  /**
   * @return Returns the user's {@link com.formationds.commons.model.type.IdentityType}
   */
  public IdentityType getIdentifier()
  {
    return identifier;
  }

  /**
   * @param identifier the user's {@link com.formationds.commons.model.type.IdentityType}
   */
  public void setIdentifier( final IdentityType identifier )
  {
    this.identifier = identifier;
  }

  /**
   * @param identifier the user's {@link com.formationds.commons.model.type.IdentityType}
   */
  public void setIdentifier( final String identifier )
  {
    this.identifier = IdentityType.lookUp( identifier );
  }

  /**
   * @return Returns the user's hashed password
   */
  public String getPasswordHash()
  {
    return passwordHash;
  }

  /**
   * @param passwordHash the user's hashed password
   */
  public void setPasswordHash( final String passwordHash )
  {
    this.passwordHash = passwordHash;
  }

  /**
   * @return Returns the secret
   */
  public String getSecret()
  {
    return secret;
  }

  /**
   * @param secret the secret
   */
  public void setSecret( final String secret )
  {
    this.secret = secret;
  }

  /**
   * @return Returns {@code true} if user is an FDS Admin
   */
  @XmlTransient
  public boolean isFdsAdmin()
  {
    return getIdentifier().equals( IdentityType.ADMIN );
  }

  /**
   * @return Returns the {@link String} representing this object
   */
  @Override
  public String toString()
  {
    return "User[ " +
           "id=" + getId() +
           ", identifier=" + getIdentifier() +
           ", isFDS Admin=" + isFdsAdmin() +
           " ]";
  }
}

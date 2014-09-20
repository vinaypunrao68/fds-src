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
import java.util.Date;

/**
 * @author ptinius
 */
@XmlRootElement
public class Snapshot
  extends ModelBase
{
  private long id;
  private String name;
  private long volumeId;
  private Date creation;

  /**
   * default constructor
   */
  public Snapshot()
  {
    super();
  }

  /**
   * @return Returns the {@link long} representing the snapshot id
   */
  public long getId()
  {
    return id;
  }

  /**
   * @param id the {@link long} representing the snapshot id
   */
  public void setId( final long id )
  {
    this.id = id;
  }

  /**
   * @return Returns the {@link String} representing the tenants name
   */
  public String getName()
  {
    return name;
  }

  /**
   * @param name the {@link String} representing the tenants name
   */
  public void setName( final String name )
  {
    this.name = name;
  }

  /**
   * @return Returns {@code long} representing the volume id
   */
  public long getVolumeId()
  {
    return volumeId;
  }

  /**
   * @param volumeId the {@code long} representing the volume id:w
   *
   */
  public void setVolumeId( final long volumeId )
  {
    this.volumeId = volumeId;
  }

  /**
   * @return Returns {@link Date} representing the creation date of this snapshot
   */
  public Date getCreation()
  {
    return creation;
  }

  /**
   * @param creation the {@link Date} representing the creation date of this snapshot
   */
  public void setCreation( final Date creation )
  {
    this.creation = creation;
  }
}

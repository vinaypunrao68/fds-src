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
import java.net.URL;
import java.util.List;

/**
 * @author ptinius
 */
@XmlRootElement
public class Stream
  extends ModelBase
{
  private static final long serialVersionUID = -1;

  private int id;
  private URL url;
  private String method;
  private List<Volume> volumes;
  private int frequency;
  private int duration;

  /**
   * default constructor
   */
  public Stream()
  {
    super();
  }

  /**
   * @return Returns {@code int} representing the stream id
   */
  public int getId()
  {
    return id;
  }

  /**
   * @param id the {@code int} representing the stream id
   */
  public void setId( final int id )
  {
    this.id = id;
  }

  /**
   * @return Returns the listening {@link URL}
   */
  public URL getUrl()
  {
    return url;
  }

  /**
   * @param url the {@link URL} that is listening
   */
  public void setUrl( final URL url )
  {
    this.url = url;
  }

  /**
   * @return Returns the {@link String} representing the method
   */
  public String getMethod()
  {
    return method;
  }

  /**
   * @param method the {@link String} representing the method
   */
  public void setMethod( final String method )
  {
    this.method = method;
  }

  /**
   * @return Returns {@link List} of {@link Volume} this stream represents
   */
  public List< Volume > getVolumes()
  {
    return volumes;
  }

  /**
   * @param volumes the {@link List} of {@link Volume} this stream will represents
   */
  public void setVolumes( final List< Volume > volumes )
  {
    this.volumes = volumes;
  }

  /**
   * @return Returns {@code int} representing the frequency at which the data was collected
   */
  public int getFrequency()
  {
    return frequency;
  }

  /**
   * @param frequency the {@code int} representing the frequency at which the data was collected
   */
  public void setFrequency( final int frequency )
  {
    this.frequency = frequency;
  }

  /**
   * @return Returns the {@code int} representing the duration
   */
  public int getDuration()
  {
    return duration;
  }

  /**
   * @param duration the {@code int} representing the duration
   */
  public void setDuration( final int duration )
  {
    this.duration = duration;
  }
}

/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
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
  extends ModelBase {
  private static final long serialVersionUID = 7993943089545141923L;

  private Integer id;
  private URL url;
  private String method;
  private List<String> volumes;
  private Integer frequency;
  private Integer duration;

  /**
   * default constructor
   */
  public Stream()
  {
    super();
  }

  /**
   * @return Returns {@link Integer} representing the stream id
   */
  public Integer getId()
  {
    return id;
  }

  /**
   * @param id the {@link Integer} representing the stream id
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
   * @return Returns {@link List} of {@link String} representing the volume names
   */
  public List<String> getVolumes()
  {
    return volumes;
  }

  /**
   * @param volumes the {@link List} of {@link String} representing the volume names
   */
  public void setVolumes( final List<String> volumes )
  {
    this.volumes = volumes;
  }

  /**
   * @return Returns {@link Integer} representing the frequency at which the data was collected
   */
  public Integer getFrequency()
  {
    return frequency;
  }

  /**
   * @param frequency the {@link Integer} representing the frequency at which the data was collected
   */
  public void setFrequency( final Integer frequency )
  {
    this.frequency = frequency;
  }

  /**
   * @return Returns the {@link Integer} representing the duration
   */
  public Integer getDuration()
  {
    return duration;
  }

  /**
   * @param duration the {@link Integer} representing the duration
   */
  public void setDuration( final Integer duration )
  {
    this.duration = duration;
  }
}

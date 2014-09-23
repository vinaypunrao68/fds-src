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

import com.formationds.commons.model.Stream;
import com.formationds.commons.model.Volume;

import java.net.URL;
import java.util.List;

/**
 * @author ptinius
 */
public class StreamBuilder {
  private int id;
  private URL url;
  private String method;
  private List<Volume> volumes;
  private int frequency;
  private int duration;

  public StreamBuilder() {
  }

  public StreamBuilder withId(int id) {
    this.id = id;
    return this;
  }

  public StreamBuilder withUrl(URL url) {
    this.url = url;
    return this;
  }

  public StreamBuilder withMethod(String method) {
    this.method = method;
    return this;
  }

  public StreamBuilder withVolumes(List<Volume> volumes) {
    this.volumes = volumes;
    return this;
  }

  public StreamBuilder withFrequency(int frequency) {
    this.frequency = frequency;
    return this;
  }

  public StreamBuilder withDuration(int duration) {
    this.duration = duration;
    return this;
  }

  public Stream build() {
    Stream stream = new Stream();
    stream.setId(id);
    stream.setUrl(url);
    stream.setMethod(method);
    stream.setVolumes(volumes);
    stream.setFrequency(frequency);
    stream.setDuration(duration);
    return stream;
  }
}

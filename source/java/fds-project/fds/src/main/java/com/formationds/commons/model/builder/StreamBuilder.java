/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.builder;

import com.formationds.commons.model.Stream;

import java.net.URL;
import java.util.List;

/**
 * @author ptinius
 */
public class StreamBuilder {
  private int id;
  private URL url;
  private String method;
  private List<String> volumes;
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

  public StreamBuilder withVolumes(List<String> volumes) {
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

/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;

import java.util.ArrayList;
import java.util.List;

/**
 * @author ptinius
 */
public class Domain
  extends ModelBase {
  private static final long serialVersionUID = 2054093416617447166L;

  private Integer id;
  private Long uuid;
  private String domain;

  private List<Node> nodes;

  private Domain( ) {
  }

  public static IDomain uuid( final Long uuid ) {
    return new Domain.Builder( uuid );
  }

  public Integer getId() {
    return id;
  }

  public void setId( final Integer id ) {
    this.id = id;
  }

  public String getDomain() {
    return domain;
  }

  public void setDomain( final String domain ) {
    this.domain = domain;
  }

  public Long getUuid( ) {
    return uuid;
  }

  public void setUuid( final Long uuid ) {
    this.uuid = uuid;
  }

  public List<Node> getNodes() {
    if( nodes == null ) {
      nodes = new ArrayList<>(  );
    }

    return nodes;
  }

  public interface IDomain {
    IBuild domain( final String domain );
  }

  public interface IBuild {
    IBuild uuid( final Long uuid );

    Domain build( );
  }

  private static class Builder
      implements IDomain, IBuild {
    private Domain instance = new Domain();

    private Builder( final Long uuid ) {
      instance.uuid = uuid;
    }

    @Override
    public IBuild domain( final String domain ) {
      instance.domain = domain;
      return this;
    }

    @Override
    public IBuild uuid( final Long uuid ) {
      instance.uuid = uuid;
      return this;
    }

    public Domain build( ) {
      return instance;
    }
  }
}

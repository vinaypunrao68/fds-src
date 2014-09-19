package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;
import com.formationds.commons.model.type.Feature;

import javax.xml.bind.annotation.XmlRootElement;
import java.util.ArrayList;
import java.util.List;

/**
 * @author ptinius
 */
@XmlRootElement
//@JsonIgnoreProperties(ignoreUnknown = true)
public class AuthenticatedUser
  extends ModelBase
{
  private static final long serialVersionUID = -1;

  private String username;
  private long userId;
  private String token;
  private List<Feature> features;

  /**
   * default constructor
   */
  public AuthenticatedUser()
  {
    super();
  }

  /**
   * @return Returns {@link String} representing the user name
   */
  public String getUsername()
  {
    return username;
  }

  /**
   * @param username the {@link String} representing the user name
   */
  public void setUsername( final String username )
  {
    this.username = username;
  }

  /**
   * @return Returns {@code long} representing the user's is
   */
  public long getUserId()
  {
    return userId;
  }

  /**
   * @param userId the {@code long} representing the user's id
   */
  public void setUserId( final long userId )
  {
    this.userId = userId;
  }

  /**
   * @return Returns {@link String} representing the authentication
   */
  public String getToken()
  {
    return token;
  }

  /**
   * @param token the {@link String} representing the authentication
   */
  public void setToken( final String token )
  {
    this.token = token;
  }

  /**
   * @return Returns {@link List} of available {@link com.formationds.commons.model.type.Feature} to this user
   */
  public List<Feature> getFeatures()
  {
    if( features == null )
    {
      features = new ArrayList<>( );
    }

    return features;
  }

  public void setFeatures( final String feature )
  {
    if( features == null )
    {
      features = new ArrayList<>( );
    }

    features.add( Feature.lookup( feature ) );
  }
}

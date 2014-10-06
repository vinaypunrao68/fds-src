/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.util.resource;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.net.URL;
import java.util.Properties;

/**
 * @author ptinius
 */
@SuppressWarnings( "UnusedDeclaration" )
public class ResourceUtils
{
  /**
   * Returns the URL of the resource on the classpath
   *
   * @param resource The resource to find
   *
   * @return Returns the resource
   *
   * @throws java.io.IOException If the resource cannot be found or read
   */
  public static URL getResourceURL( String resource )
    throws IOException
  {
    URL url = null;
    ClassLoader loader = ResourceUtils.class.getClassLoader();
    if( loader != null )
    {
      url = loader.getResource( resource );
    }
    if( url == null )
    {
      url = ClassLoader.getSystemResource( resource );
    }
    if( url == null )
    {
      throw new IOException( "Could not find resource " + resource );
    }
    return url;
  }

  /** */

  /**
   * Returns the URL of the resource on the classpath
   *
   * @param loader The class loader used to load the resource
   * @param resource The resource to find
   *
   * @return Returns the resource
   *
   * @throws java.io.IOException If the resource cannot be found or read
   */
  public static URL getResourceURL( ClassLoader loader, String resource )
    throws IOException
  {
    URL url = null;
    if( loader != null )
    {
      url = loader.getResource( resource );
    }
    if( url == null )
    {
      url = ClassLoader.getSystemResource( resource );
    }
    if( url == null )
    {
      throw new IOException( "Could not find resource " + resource );
    }
    return url;
  }

  /**
   * @param resource The resource to find
   *
   * @return Returns the resource
   *
   * @throws java.io.IOException If the resource cannot be found or read
   */
  public static InputStream getResourceAsStream( String resource )
    throws IOException
  {
    InputStream in = null;
    ClassLoader loader = ResourceUtils.class.getClassLoader();
    if( loader != null )
    {
      in = loader.getResourceAsStream( resource );
    }
    if( in == null )
    {
      in = ClassLoader.getSystemResourceAsStream( resource );
    }
    if( in == null )
    {
      throw new IOException( "Could not find resource " + resource );
    }
    return in;
  }

  /**
   * @param loader The class loader used to load the resource
   * @param resource The resource to find
   *
   * @return Returns the resource
   *
   * @throws java.io.IOException If the resource cannot be found or read
   */
  public static InputStream getResourceAsStream( ClassLoader loader,
                                                 String resource )
    throws IOException
  {
    InputStream in = null;
    if( loader != null )
    {
      in = loader.getResourceAsStream( resource );
    }
    if( in == null )
    {
      in = ClassLoader.getSystemResourceAsStream( resource );
    }
    if( in == null )
    {
      throw new IOException( "Could not find resource " + resource );
    }
    return in;
  }

  /**
   * @param resource The resource to find
   *
   * @return Returns the resource
   *
   * @throws java.io.IOException If the resource cannot be found or read
   */
  public static Properties getResourceAsProperties( String resource )
    throws IOException
  {
    Properties props = new Properties();
    InputStream in = getResourceAsStream( resource );
    props.load( in );
    in.close();
    return props;
  }

  /**
   * @param loader The class loader used to load the resource
   * @param resource The resource to find
   *
   * @return Returns the resource
   *
   * @throws java.io.IOException If the resource cannot be found or read
   */
  public static Properties getResourceAsProperties( ClassLoader loader,
                                                    String resource )
    throws IOException
  {
    Properties props = new Properties();
    InputStream in = getResourceAsStream( loader, resource );
    props.load( in );
    in.close();
    return props;
  }

  /**
   * Returns a resource on the classpath as a Reader object
   *
   * @param resource The resource to find
   *
   * @return The resource
   *
   * @throws java.io.IOException If the resource cannot be found or read
   */
  public static InputStreamReader getResourceAsReader( String resource )
    throws IOException
  {
    return new InputStreamReader( getResourceAsStream( resource ) );
  }

  /**
   * @param loader The class loader used to load the resource
   * @param resource The resource to find
   *
   * @return Returns the resource
   *
   * @throws java.io.IOException If the resource cannot be found or read
   */
  public static Reader getResourceAsReader( ClassLoader loader,
                                            String resource )
    throws IOException
  {
    return new InputStreamReader( getResourceAsStream( loader, resource ) );
  }

  /**
   * @param resource The resource to find
   *
   * @return Returns the resource
   *
   * @throws java.io.IOException If the resource cannot be found or read
   */
  public static File getResourceAsFile( String resource )
    throws IOException
  {
    return new File( getResourceURL( resource ).getFile() );
  }

  /**
   * @param loader The class loader used to load the resource
   * @param resource The resource to find
   *
   * @return Returns the resource
   *
   * @throws java.io.IOException If the resource cannot be found or read
   */
  public static File getResourceAsFile( ClassLoader loader, String resource )
    throws IOException
  {
    return new File( getResourceURL( loader, resource ).getFile() );
  }
}

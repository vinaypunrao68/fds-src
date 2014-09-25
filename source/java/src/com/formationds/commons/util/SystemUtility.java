/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.util;

import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.io.LineNumberReader;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;

/**
 * @author ptinius
 */
@SuppressWarnings( "unused" )
public class SystemUtility
{
  private static final String OS_NAME = "os.name";

  private static String osReleaseFile = null;

  static
  {
    if( isLinux() )
    {
      if( Files.exists( Paths.get( "/etc/redhat-release" ) ) )
      {
        osReleaseFile = "/etc/redhat-release";
      }
      else if( Files.exists( Paths.get( "/etc/SuSE-release" ) ) )
      {
        osReleaseFile = "/etc/SuSE-release";
      }
      else if( Files.exists( Paths.get( "/etc/os-release" ) ) )
      {
        osReleaseFile = "/etc/os-release";
      }
    }
  }

  /**
   * @return Returns a {@link java.util.List} of {@link String} representing the root files systems.
   */
  public static List<String> getRoots()
  {
    List<String> roots = new ArrayList<>();

    File[] drives = File.listRoots(); // Get list of names
    for( File drive : drives )
    {
      roots.add( drive.toString() );
    }

    return roots;
  }

  /**
   * @return Returns the well defined platform name
   */
  public static String getPlatform()
  {
    return System.getProperty( OS_NAME );
  }

  /**
   * @return true if platform is windows, otherwise false is returned.
   */
  public static boolean isWindows()
  {
    return ( System.getProperty( OS_NAME ).contains( "Windows" ) );
  }

  /**
   * @return true if platform is linux, otherwise false is returned.
   */
  public static boolean isLinux()
  {
    return ( System.getProperty( OS_NAME ).contains( "Linux" ) );
  }

  /**
   * @return true if platform is Redhat linux, otherwise false is returned.
   */
  public static boolean isRedhatLinux()
  {
    return isLinux() &&
           ( osReleaseFile != null ) &&
           osReleaseFile.equals( "/etc/redhat-release" );

  }

  /**
   * @return true if platform is SuSE linux, otherwise false is returned.
   */
  public static boolean isSuSELinux()
  {
    return isLinux() &&
           ( osReleaseFile != null ) &&
           osReleaseFile.equals( "/etc/SuSE-release" );

  }

  /**
   * @return true if platform is ubuntu linux, otherwise false is returned.
   */
  public static boolean isUbuntuLinux()
  {
    return isLinux() &&
           ( osReleaseFile != null ) &&
           osReleaseFile.equals( "/etc/os-release" );
  }

  /**
   * @return true if platform is solaris, otherwise false is returned.
   */
  public static boolean isSolaris()
  {
    String os = System.getProperty( OS_NAME );
    return ( os.contains( "SunOS" ) ) || ( os.contains( "Solaris" ) );
  }

  /**
   * @return true if platform is aix, otherwise false is returned.
   */
  public static boolean isAix()
  {
    return ( System.getProperty( OS_NAME ).contains( "AIX" ) );
  }

  /**
   * @return true if platform is irix, otherwise false is returned.
   */
  public static boolean isIrix()
  {
    return ( System.getProperty( OS_NAME ).contains( "IRIX" ) );
  }

  /**
   * @return true if platform is hpux, otherwise false is returned.
   */
  public static boolean isHpux()
  {
    return ( System.getProperty( OS_NAME ).contains( "HP-UX" ) );
  }

  /**
   * @return true if platform is mac, otherwise false is returned.
   */
  public static boolean isMac()
  {
    return ( System.getProperty( OS_NAME ).contains( "Mac OS X" ) );
  }

  /**
   * @return Returns the operating system name
   */
  public static String getOS()
  {
    String osType = System.getProperty( OS_NAME );
    if( isLinux() )
    {
      if( osReleaseFile != null )
      {
        switch( osReleaseFile )
        {
          case "/etc/redhat-release":
            osType = "Redhat " + System.getProperty( OS_NAME );
            break;
          case "/etc/SuSE-release":
            osType = "SuSE " + System.getProperty( OS_NAME );
            break;
          case "/etc/os-release":
            osType = getOSDescription() + " " + System.getProperty( OS_NAME );
            break;
        }
      }
    }

    return osType;
  }

  /**
   *
   * @return Returns the operating system description
   */
  public static String getOSDescription()
  {
    String osDesc = System.getProperty( OS_NAME );
    if( isLinux() )
    {
      if( osReleaseFile != null )
      {
        LineNumberReader r = null;
        try
        {
          r = new LineNumberReader( new FileReader( osReleaseFile ) );
          String line;
          while( ( line = r.readLine() )  != null )
          {
            if( r.getLineNumber() == 1 )
            {
              osDesc = line;
              break;
            }
          }
        }
        catch( IOException e )
        {
          System.err.println( e.getMessage() );
        }
        finally
        {
          if( r != null )
          {
            try { r.close(); } catch( Exception ignore ) {}
          }
        }
      }
    }

    return osDesc;
  }

  /**
   * @return Returns {@link String} representing the current working directory
   */
  public static String getCurrentWorkingDirectory()
  {
    return System.getProperty( "user.dir" );
  }

  /**
   * @param integer the {@link Integer}
   *
   * @return the {@link String} representing the binary number with leading zeros
   */
  public static String toBinaryString( final int integer )
  {
    return String.format( "%8s" ,
                          Integer.toBinaryString( integer & 0xFF ) )
                 .replace( ' ', '0' );
  }

  /**
   * Constructs a com.formationds.commons.util.SystemUtility object.
   */
  private SystemUtility()
  {
    super();
  }
}

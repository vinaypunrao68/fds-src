/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.util;

import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.LogManager;

import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.regex.PatternSyntaxException;

/**
 * @author ptinius
 */
public class RegExUtil {
  private static final Logger logger =
    LogManager.getLogger( RegExUtil.class );

  public static final String WHITE_SPACE = "\\p{Space}";
  public static final String TAB = "\t";

  /**
   * @param regEx the {@link String} representation of the regular expression
   *
   * @return Returns the regular expression {@link java.util.regex.Pattern}
   */
  public static Pattern compileRegEx( String regEx ) {
    Pattern pattern = null;
    try {
      pattern = Pattern.compile( regEx );
    } catch( PatternSyntaxException pse ) {
      logger.error( "compileRegEx( " + regEx + " )", pse );
    }

    return pattern;
  }

  /**
   * @param regEx the {@link String} representation of the regular expression
   * @param flags the match flags, a bit mask that may include {@link
   *              java.util.regex.Pattern#CASE_INSENSITIVE}, {@link
   *              java.util.regex.Pattern#MULTILINE}, {@link
   *              java.util.regex.Pattern#DOTALL}, {@link java.util.regex.Pattern#UNICODE_CASE},
   *              {@link java.util.regex.Pattern#CANON_EQ}, {@link
   *              java.util.regex.Pattern#UNIX_LINES}, {@link
   *              java.util.regex.Pattern#LITERAL}, {@link java.util.regex.Pattern#UNICODE_CHARACTER_CLASS}
   *              and {@link java.util.regex.Pattern#COMMENTS}
   *
   * @return Returns the regular expression {@link java.util.regex.Pattern}
   */
  @SuppressWarnings( "UnusedDeclaration" )
  public static Pattern compileRegEx( String regEx, int flags ) {
    Pattern pattern = null;
    try {
      //noinspection MagicConstant
      pattern = Pattern.compile( regEx, flags );
    } catch( PatternSyntaxException pse ) {
      logger.error( "compileRegEx( " + regEx + " )", pse );
    }

    return pattern;
  }

  /**
   * @param pattern the {@link String} representation of the regular expression
   *                pattern
   * @param string  the {@link String} to apply the regular expression pattern
   *                against
   * @param flags   the match flags, a bit mask that may include {@link
   *                java.util.regex.Pattern#CASE_INSENSITIVE}, {@link
   *                java.util.regex.Pattern#MULTILINE}, {@link
   *                java.util.regex.Pattern#DOTALL}, {@link java.util.regex.Pattern#UNICODE_CASE},
   *                {@link java.util.regex.Pattern#CANON_EQ}, {@link
   *                java.util.regex.Pattern#UNIX_LINES}, {@link
   *                java.util.regex.Pattern#LITERAL}, {@link
   *                java.util.regex.Pattern#UNICODE_CHARACTER_CLASS} and {@link
   *                java.util.regex.Pattern#COMMENTS}
   *
   * @return Returns the regular expression matches {@link
   * java.util.regex.Matcher}
   */
  public static Matcher matches( final String pattern, final String string, int flags ) {
    Matcher matcher = null;
    try {
      matcher = Pattern.compile( pattern, flags )
                       .matcher( string );
    } catch( PatternSyntaxException pse ) {
      logger.error( "matches( '" + pattern + "', '" + string + "' )", pse );
    }

    return matcher;
  }

  /**
   * @param pattern the {@link String} representation of the regular expression
   *                pattern
   * @param string  the {@link String} to apply the regular expression pattern
   *                against
   *
   * @return Returns the regular expression matches {@link
   * java.util.regex.Matcher}
   */
  public static Matcher matches( final String pattern, final String string ) {
    Matcher matcher = null;
    try {
      matcher = Pattern.compile( pattern )
                       .matcher( string );
    } catch( PatternSyntaxException pse ) {
      logger.error( "matches( '" + pattern + "', '" + string + "' )", pse );
    }

    return matcher;
  }

  /**
   * @param matcher the {@link java.util.regex.Matcher}
   */
  public static void dumpMather( final Matcher matcher ) {
    for( int i = 1;
         i <= matcher.groupCount();
         i++ ) {
      System.out.println( i + ": '" + matcher.group( i ) + "'" );
    }
  }
}

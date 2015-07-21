package com.formationds.commons.util;

/**
 * General string-handling utilities.
 */
public final class Strings
{
    /**
     * Create a Java string.
     * 
     * @param value The raw string.
     * 
     * @return A properly quoted and escaped Java string.
     */
    public static String javaString(String value)
    {
        if (value == null) return "<null>";

        return "\"" + value.replace("\\", "\\\\")
                           .replace("\t", "\\t")
                           .replace("\b", "\\b")
                           .replace("\n", "\\n")
                           .replace("\r", "\\r")
                           .replace("\'", "\\'")
                           .replace("\"", "\\\"") + "\"";
    }

    /**
     * Compare two strings, where {@code null} will not throw errors.
     * 
     * @param a The first string to compare.
     * @param b The second string to compare.
     * 
     * @return Whether {@code a} is equivalent to {@code b}.
     */
    public static boolean nullTolerantEquals(String a, String b)
    {
        if (a == null)
        {
            return b == null;
        }
        else if (b == null)
        {
            return false;
        }
        else
        {
            return a.equals(b);
        }
    }
    
    /**
     * Determine if a string is prefixed with another, where {@code null} will not throw errors.
     * 
     * @param text The string to check.
     * @param prefix The prefix to check for.
     * 
     * @return Whether {@code text} starts with {@code prefix}.
     */
    public static boolean nullTolerantStartsWith(String text, String prefix)
    {
        if (text == null)
        {
            // If two strings compare equal, as they do in nullTolerantEquals, we should say they
            // share the same prefix.
            return prefix == null;
        }
        else if (prefix == null)
        {
            // Just as String.startsWith("") == true for any text.
            return true;
        }
        else
        {
            return text.startsWith(prefix);
        }
    }
    
    /**
     * Prevent instantiation.
     */
    private Strings()
    {
        throw new UnsupportedOperationException("Instantiating a utility class.");
    }
}

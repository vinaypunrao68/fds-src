package com.formationds.commons.util;

public final class Strings
{
    public static String javaString(String value)
    {
        if (value == null) return "null";

        return "\"" + value.replace("\\", "\\\\")
                           .replace("\t", "\\t")
                           .replace("\b", "\\b")
                           .replace("\n", "\\n")
                           .replace("\r", "\\r")
                           .replace("\'", "\\'")
                           .replace("\"", "\\\"") + "\"";
    }

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
    
    private Strings()
    {
        throw new UnsupportedOperationException("Instantiating a utility class.");
    }
}

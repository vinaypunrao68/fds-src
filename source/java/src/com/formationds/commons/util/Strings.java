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

    private Strings()
    {
        throw new UnsupportedOperationException("Instantiating a utility class.");
    }
}

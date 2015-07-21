package com.formationds.iodriver.operations;

import static com.formationds.commons.util.Strings.javaString;

import java.util.AbstractMap.SimpleImmutableEntry;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.patterns.VisitableWithArg;
import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

/**
 * An operation that may be executed.
 * 
 * @param <ThisT> The implementing class.
 * @param <EndpointT> The type of endpoint this operation can run on.
 */
// @eclipseFormat:off
public abstract class Operation<ThisT extends Operation<ThisT, EndpointT>,
                                EndpointT extends Endpoint<EndpointT, ThisT>>
implements VisitableWithArg<EndpointT, ThisT, AbstractWorkflowEventListener, ExecutionException>
//@eclipseFormat:on
{
    @Override
    public String toString()
    {
        String membersAsString;
        try
        {
            membersAsString = toStringMembers().map(kvp -> kvp.getKey() + ": "
                                                           + kvp.getValue())
                                               .collect(Collectors.joining(", "));
        }
        catch (Exception e)
        {
            membersAsString = e.getClass().getSimpleName() + ": " + e.getMessage();
        }
        
        return getClass().getSimpleName() + "(" + membersAsString + ")";
    }
    
    /**
     * Convert members to string format, suitable for embedding in
     * "Typename(memberName: value, ...)". Each member is individually converted, and it it is up
     * to the final .toString() method to join as appropriate. Keys and values should be properly
     * quoted for string form, i.e. double quotes and escapes around strings, no quotes around
     * numerics, pseudo-constructor format for complex types.
     * 
     * Note to implementers: call superclass method and concat values.
     * 
     * @return The members of this class as strings.
     */
    protected Stream<SimpleImmutableEntry<String, String>> toStringMembers()
    {
        return Stream.empty();
    }
    
    protected static SimpleImmutableEntry<String, String> memberToString(String name, String value)
    {
        if (name == null) throw new NullArgumentException("name");
        
        return new SimpleImmutableEntry<>(javaString(name), javaString(value));
    }

    protected static SimpleImmutableEntry<String, String> memberToString(String name, Object value)
    {
        if (name == null) throw new NullArgumentException("name");
        
        if (value instanceof String || value == null)
        {
            return memberToString(name, (String)value);
        }
        else
        {
            return new SimpleImmutableEntry<>(javaString(name), value.toString());
        }
    }
}

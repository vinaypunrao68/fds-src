package com.formationds.iodriver.operations;

import java.util.AbstractMap.SimpleImmutableEntry;
import java.util.function.Supplier;
import java.util.stream.Stream;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.WorkloadContext;
import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.events.Event;

public class FireEvent<EventT extends Event<T>, T> extends AbstractOperation
{
    public FireEvent(Supplier<? extends EventT> eventSupplier)
    {
        if (eventSupplier == null) throw new NullArgumentException("eventSupplier");

        _eventSupplier = eventSupplier;
    }

    @Override
    // @eclipseFormat:off
    public void accept(Endpoint endpoint,
                       WorkloadContext context) throws ExecutionException
    // @eclipseFormat:on
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (context == null) throw new NullArgumentException("context");

        context.sendIfRegistered(_eventSupplier.get());
    }

    @Override
    public Stream<SimpleImmutableEntry<String, String>> toStringMembers()
    {
        return Stream.concat(super.toStringMembers(),
                             Stream.of(memberToString("eventSupplier", _eventSupplier)));
    }
    
    /**
     * The bucket to report begin for.
     */
    private final Supplier<? extends EventT> _eventSupplier;
}

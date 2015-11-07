package com.formationds.commons.patterns;

import java.io.Closeable;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.atomic.AtomicBoolean;

import com.formationds.commons.Fds;
import com.formationds.commons.NullArgumentException;

public final class Subject<T> implements Observable<T>
{
    public Subject()
    {
        _observers = new HashMap<>();
    }

    @Override
    public Closeable register(Observer<? super T> observer)
    {
        if (observer == null) throw new NullArgumentException("observer");

        synchronized(_observers)
        {
            byte[] key = new byte[4];
            for (;;)
            {
                Fds.Random.nextBytes(key);
                if (!_observers.containsKey(key))
                {
                    _observers.put(key, observer);
                    return new RegistrationToken(key, this);
                }
            }
        }
    }

    public void send(T message)
    {
        if (message == null) throw new NullArgumentException("message");

        synchronized(_observers)
        {
            for (Observer<? super T> observer : _observers.values())
            {
                observer.observe(message);
            }
        }
    }

    @Override
    public boolean unregister(byte[] registrationKey)
    {
        if (registrationKey == null) throw new NullArgumentException("registrationKey");

        Observer<? super T> removed;
        synchronized(_observers)
        {
            removed = _observers.remove(registrationKey);
        }
        return removed != null;
    }

    private static class RegistrationToken implements Closeable
    {
        public RegistrationToken(byte[] key, Subject<?> subject)
        {
            if (key == null) throw new NullArgumentException("key");
            if (subject == null) throw new NullArgumentException("subject");
            
            _closed = new AtomicBoolean(false);
            _key = key;
            _subject = subject;
        }
        
        @Override
        public void close() throws IOException
        {
            if (_closed.compareAndSet(false, true))
            {
                _subject.unregister(_key);
            }
        }
        
        @Override
        protected void finalize() throws Throwable
        {
            try
            {
                close();
            }
            finally
            {
                super.finalize();
            }
        }
        
        private final AtomicBoolean _closed;
        
        private final byte[] _key;
        
        private final Subject<?> _subject;
    }
    
    private final Map<byte[], Observer<? super T>> _observers;
}

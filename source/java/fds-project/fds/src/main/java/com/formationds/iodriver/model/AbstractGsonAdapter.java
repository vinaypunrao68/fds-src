package com.formationds.iodriver.model;

import static com.formationds.commons.util.Strings.javaString;

import java.io.IOException;
import java.time.Duration;
import java.time.Instant;
import java.time.format.DateTimeParseException;
import java.util.AbstractMap.SimpleImmutableEntry;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.util.functional.ExceptionThrowingBiConsumer;
import com.formationds.commons.util.functional.ExceptionThrowingFunction;
import com.google.gson.TypeAdapter;
import com.google.gson.stream.JsonReader;
import com.google.gson.stream.JsonToken;
import com.google.gson.stream.JsonWriter;

public abstract class AbstractGsonAdapter<T, ContextT> extends TypeAdapter<T>
{
    @Override
    public T read(JsonReader in) throws IOException
    {
        if (in == null) throw new NullArgumentException("in");
        
        ContextT context = newContext();
        readAllValues(in, context);
        return build(context);
    }
    
    @Override
    public void write(JsonWriter out, T source) throws IOException
    {
        if (out == null) throw new NullArgumentException("out");
        if (source == null) throw new NullArgumentException("source");
        
        boolean originalSerializeNulls = out.getSerializeNulls();
        out.setSerializeNulls(true);
        try
        {
            out.beginObject();
            writeValues(source, out);
            out.endObject();
        }
        finally
        {
            out.setSerializeNulls(originalSerializeNulls);
        }
    }
    
    protected abstract T build(ContextT context);

    protected abstract ContextT newContext();
    
    protected void readAllValues(JsonReader in,
                                 ContextT context) throws IOException
    {
        if (in == null) throw new NullArgumentException("in");
        if (context == null) throw new NullArgumentException("context");
        
        in.beginObject();
        while (in.hasNext())
        {
            readValue(in.nextName(), in, context);
        }
        in.endObject();
    }
    
    protected abstract void readValue(String name,
                                      JsonReader in,
                                      ContextT context) throws IOException;

    protected static Boolean readBooleanNullable(JsonReader in) throws IOException
    {
        if (in == null) throw new NullArgumentException("in");
        
        return readNullable(in, reader -> reader.nextBoolean());
    }
    
    protected static Duration readDuration(JsonReader in) throws IOException
    {
        if (in == null) throw new NullArgumentException("in");
        
        String durationString = in.nextString();
        try
        {
            return Duration.parse(durationString);
        }
        catch (DateTimeParseException e)
        {
            throw new IOException("Error parsing " + Duration.class.getName() + ": "
                                  + javaString(durationString) + ".",
                                  e);
        }
    }
    
    protected static Duration readDurationNullable(JsonReader in) throws IOException
    {
        if (in == null) throw new NullArgumentException("in");

        return readNullable(in, AbstractGsonAdapter::readDuration);
    }
    
    protected static Instant readInstant(JsonReader in) throws IOException
    {
        if (in == null) throw new NullArgumentException("in");
        
        String instantString = in.nextString();
        try
        {
            return Instant.parse(instantString);
        }
        catch (DateTimeParseException e)
        {
            throw new IOException("Error parsing " + Instant.class.getName() + ": "
                                  + javaString(instantString) + ".",
                                  e);
        }
    }
    
    protected static Instant readInstantNullable(JsonReader in) throws IOException
    {
        if (in == null) throw new NullArgumentException("in");

        return readNullable(in, AbstractGsonAdapter::readInstant);
    }
    
    protected static Integer readIntegerNullable(JsonReader in) throws IOException
    {
        if (in == null) throw new NullArgumentException("in");
        
        return readNullable(in, reader -> reader.nextInt());
    }
    
    protected static <T> List<T> readList(
            JsonReader in,
            ExceptionThrowingFunction<? super JsonReader, ? extends T, IOException> elementCreator)
            throws IOException
    {
        if (in == null) throw new NullArgumentException("in");
        if (elementCreator == null) throw new NullArgumentException("elementCreator");

        in.beginArray();
        List<T> list = new ArrayList<>();
        while (in.hasNext())
        {
            list.add(elementCreator.apply(in));
        }
        in.endArray();
        
        return list;
    }
    
    protected static Long readLongNullable(JsonReader in) throws IOException
    {
        if (in == null) throw new NullArgumentException("in");
        
        return readNullable(in, reader -> reader.nextLong());
    }
    
    protected static <KeyT, ValueT> Map<KeyT, ValueT> readMap(
            JsonReader in,
            ExceptionThrowingFunction<? super JsonReader,
                                      ? extends Entry<? extends KeyT, ? extends ValueT>,
                                      IOException> elementCreator)
            throws IOException
    {
        if (in == null) throw new NullArgumentException("in");
        if (elementCreator == null) throw new NullArgumentException("elementCreator");

        in.beginObject();
        Map<KeyT, ValueT> map = new HashMap<>();
        while (in.hasNext())
        {
            Entry<? extends KeyT, ? extends ValueT> element = elementCreator.apply(in);
            map.put(element.getKey(), element.getValue());
        }
        in.endObject();
        
        return map;
    }
    
    protected static <KeyT, ValueT> Map<KeyT, ValueT> readMapNullable(
            JsonReader in,
            ExceptionThrowingFunction<? super JsonReader,
                                      ? extends Entry<? extends KeyT, ? extends ValueT>,
                                      IOException> elementCreator)
            throws IOException
    {
        if (in == null) throw new NullArgumentException("in");
        if (elementCreator == null) throw new NullArgumentException("elementCreator");

        return readNullable(in, reader -> readMap(reader, elementCreator));
    }
    
    protected static SimpleImmutableEntry<String, String> readMapStringEntry(JsonReader in)
            throws IOException
    {
        if (in == null) throw new NullArgumentException("in");
        
        String key = in.nextName();
        String value = in.nextString();
        
        return new SimpleImmutableEntry<>(key, value);
    }
    
    protected static <T> T readNullable(
            JsonReader in,
            ExceptionThrowingFunction<JsonReader, T, IOException> innerRead)
            throws IOException
    {
        if (in == null) throw new NullArgumentException("in");
        if (innerRead == null) throw new NullArgumentException("innerRead");
        
        if (in.peek().equals(JsonToken.NULL))
        {
            in.nextNull();
            return null;
        }
        else
        {
            return innerRead.apply(in);
        }
    }
    
    // FIXME: writeMapNullable
    protected static <KeyT, ValueT> void writeNullableMap(
            JsonWriter out,
            Map<KeyT, ValueT> map,
            ExceptionThrowingBiConsumer<JsonWriter,
                                        ? super Entry<KeyT, ValueT>,
                                        IOException> entryWriter)
            throws IOException
    {
        if (out == null) throw new NullArgumentException("out");
        if (entryWriter == null) throw new NullArgumentException("entryWriter");
        
        if (map == null)
        {
            out.nullValue();
        }
        else
        {
            out.beginObject();
            for (Entry<KeyT, ValueT> entry : map.entrySet())
            {
                entryWriter.accept(out, entry);
            }
            out.endObject();
        }
    }

    protected static void writeMapStringEntry(JsonWriter out,
                                              Entry<String, String> entry) throws IOException
    {
        if (out == null) throw new NullArgumentException("out");
        if (entry == null) throw new NullArgumentException("entry");
        
        out.name(entry.getKey());
        out.value(entry.getValue());
    }

    protected static <T> void writeNullable(JsonWriter out,
                                            T value,
                                            ExceptionThrowingBiConsumer<JsonWriter,
                                                                        ? super T,
                                                                        IOException> writer)
            throws IOException
    {
        if (out == null) throw new NullArgumentException("out");
        if (writer == null) throw new NullArgumentException("writer");
        
        if (value == null)
        {
            out.nullValue();
        }
        else
        {
            writer.accept(out, value);
        }
    }
    
    protected static void writeNullableBoolean(JsonWriter out, Boolean value) throws IOException
    {
        if (out == null) throw new NullArgumentException("out");
        
        writeNullable(out, value, (w, v) -> w.value(v));
    }
    
    protected static void writeNullableDuration(JsonWriter out, Duration value) throws IOException
    {
        if (out == null) throw new NullArgumentException("out");

        writeNullable(out, value, (w, v) -> w.value(v.toString()));
    }
    
    protected static void writeNullableInstant(JsonWriter out, Instant value) throws IOException
    {
        if (out == null) throw new NullArgumentException("out");

        writeNullable(out, value, (w, v) -> w.value(v.toString()));
    }
    
    protected abstract void writeValues(T source, JsonWriter out) throws IOException;
}

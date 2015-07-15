package com.formationds.fdsdiff;

import static com.formationds.commons.util.Strings.javaString;

import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Arrays;
import java.util.Map.Entry;
import java.util.stream.Stream;

import javax.xml.bind.DatatypeConverter;

import com.formationds.commons.NullArgumentException;


public class FullObjectManifest extends BasicObjectManifest
{
    public static class Builder<ThisT extends Builder<ThisT>>
            extends BasicObjectManifest.Builder<ThisT>
    {
        @Override
        public FullObjectManifest build()
        {
            validate();
            
            return new FullObjectManifest(getThis());
        }
        
        public final byte[] getContent()
        {
            return _content;
        }
        
        public final ThisT setContent(byte[] value)
        {
            _content = value;
            return getThis();
        }
        
        @Override
        protected void validate()
        {
            super.validate();
            
            if (_content == null) throw new IllegalStateException("content cannot be null.");
            
            if (_content.length != getSize())
            {
                throw new IllegalStateException("content is " + _content.length + " bytes, but size"
                                                + " is " + getSize() + ".");
            }
            
            MessageDigest md5Summer;
            try
            {
                md5Summer = MessageDigest.getInstance("MD5");
            }
            catch (NoSuchAlgorithmException e)
            {
                throw new IllegalStateException("MD5 algorithm is not supported.", e);
            }
            
            byte[] actualMd5 = md5Summer.digest(_content);
            byte[] builderMd5 = getMd5();
            if (!Arrays.equals(actualMd5, builderMd5))
            {
                throw new IllegalStateException(
                        "MD5 provided does not match content: "
                        + "Actual(" + DatatypeConverter.printHexBinary(actualMd5) + "), "
                        + "Provided(" + DatatypeConverter.printHexBinary(builderMd5) + ").");
            }
        }
        
        private byte[] _content;
    }
    
    @Override
    public boolean equals(Object other)
    {
        if (!super.equals(other))
        {
            return false;
        }
        
        FullObjectManifest typedOther = (FullObjectManifest)other;
        return Arrays.equals(_content, typedOther._content);
    }
    
    @Override
    public int hashCode()
    {
        int hash = super.hashCode();
        
        hash = hash * 23 ^ Arrays.hashCode(_content);
        
        return hash;
    }
    
    protected FullObjectManifest(Builder<?> builder)
    {
        super(builder);
        
        if (builder == null) throw new NullArgumentException("builder");
        
        _content = builder.getContent();
    }
    
    @Override
    protected Stream<Entry<String, String>> toJsonStringMembers()
    {
        return Stream.concat(
                super.toStringMembers(),
                Stream.of(
                        memberToJsonString(
                                "content",
                                javaString(DatatypeConverter.printBase64Binary(_content)))));
    }
    
    @Override
    protected Stream<Entry<String, String>> toStringMembers()
    {
        return Stream.concat(
                super.toStringMembers(),
                Stream.of(memberToString("content",
                                         DatatypeConverter.printBase64Binary(_content))));
    }
    
    private final byte[] _content;
}

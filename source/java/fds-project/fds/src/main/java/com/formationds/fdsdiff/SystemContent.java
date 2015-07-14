package com.formationds.fdsdiff;

import static com.formationds.commons.util.Strings.javaString;

import java.time.Duration;
import java.time.Instant;
import java.util.Collection;
import java.util.HashSet;
import java.util.Set;

import com.formationds.client.v08.model.DataProtectionPolicy;
import com.formationds.client.v08.model.Tenant;
import com.formationds.client.v08.model.User;
import com.formationds.client.v08.model.Volume;
import com.formationds.client.v08.model.VolumeAccessPolicy;
import com.formationds.commons.NullArgumentException;

public class SystemContent
{
    public SystemContent()
    {
        _tenants = new HashSet<>();
        _users = new HashSet<>();
        _volumes = new HashSet<>();
    }
    
    public void setTenants(Collection<Tenant> value)
    {
        if (value == null) throw new NullArgumentException("value");
        
        _tenants.clear();
        _tenants.addAll(value);
    }
    
    public void setUsers(Collection<User> value)
    {
        if (value == null) throw new NullArgumentException("value");
        
        _users.clear();
        _users.addAll(value);
    }
    
    public void setVolumes(Collection<Volume> value)
    {
        if (value == null) throw new NullArgumentException("value");
        
        _volumes.clear();
        _volumes.addAll(value);
    }
    
    @Override
    public String toString()
    {
        return "{"
               + "  \"tenants\": [\n"
               + String.join(",\n", _tenants.stream()
                                            .map(t -> "    " + toJson(t))
                                            .<String>toArray(size -> new String[size]))
               + "  ],\n"
               + String.join(",\n", _users.stream()
                                          .map(u -> "    " + toJson(u))
                                          .<String>toArray(size -> new String[size]))
               + "}";
    }
    
    private final Set<Tenant> _tenants;
    
    private final Set<User> _users;
    
    private final Set<Volume> _volumes;
    
    private static String toJson(DataProtectionPolicy dataProtectionPolicy)
    {
        if (dataProtectionPolicy == null) throw new NullArgumentException("dataProtectionPolicy");
        
        return "{ "
               + toJsonValue("presetId", dataProtectionPolicy.getPresetId())
               + ", " + toJsonValue("commitLogRetention", dataProtectionPolicy.getCommitLogRetention())
               + ", " + toJsonValue("snapshotPolicies", dataProtectionPolicy.getSnapshotPolicies())
               + " }";
    }
    
    private static String toJson(Tenant tenant)
    {
        if (tenant == null) throw new NullArgumentException("tenant");
        
        return "{ "
               + toJsonValue("id", tenant.getId())
               + ", " + toJsonValue("name", tenant.getName())
               + " }";
    }
    
    private static String toJson(User user)
    {
        if (user == null) throw new NullArgumentException("user");
        
        return "{ "
               + toJsonValue("id", user.getId())
               + toJsonValue("tenant", user.getTenant().getId())
               + toJsonValue("name", user.getName())
               + toJsonValue("roleId", user.getRoleId())
               + " }";
    }
    
    private static String toJson(Volume volume)
    {
        if (volume == null) throw new NullArgumentException("volume");
        
        return "{ "
               + toJsonValue("id", volume.getId()) + ", "
               + toJsonValue("application", volume.getApplication()) + ", "
               + toJsonValue("name", volume.getName()) + ", "
               + toJsonValue("volumeAccessPolicy", volume.getAccessPolicy()) + ", "
               + toJsonValue("created", volume.getCreated()) + ", "
               + toJsonValue("dataProtectionPolicy", volume.getDataProtectionPolicy())
               + " }";
    }
    
    private static String toJson(VolumeAccessPolicy volumeAccessPolicy)
    {
        if (volumeAccessPolicy == null) throw new NullArgumentException("volumeAccessPolicy");
        
        return "{ "
               + toJsonValue("isExclusiveRead", volumeAccessPolicy.isExclusiveRead()) + ", "
               + toJsonValue("isExclusiveWrite", volumeAccessPolicy.isExclusiveWrite())
               + " }";
    }
    
    private static String toJsonValue(String name, boolean value)
    {
        if (name == null) throw new NullArgumentException("name");
        
        return javaString(name) + ": " + value;
    }
    
    private static String toJsonValue(String name, DataProtectionPolicy value)
    {
        if (name == null) throw new NullArgumentException("name");
        if (value == null) throw new NullArgumentException("value");

        return javaString(name) + ": " + toJson(value);
    }
    
    private static String toJsonValue(String name, Duration value)
    {
        if (name == null) throw new NullArgumentException("name");
        if (value == null) throw new NullArgumentException("value");

        return javaString(name) + ": " + javaString(value.toString());
    }
    
    private static String toJsonValue(String name, Instant value)
    {
        if (name == null) throw new NullArgumentException("name");
        if (value == null) throw new NullArgumentException("value");

        return javaString(name) + ": " + javaString(value.toString());
    }
    
    private static String toJsonValue(String name, Long value)
    {
        if (name == null) throw new NullArgumentException("name");
        if (value == null) throw new NullArgumentException("value");
        
        return javaString(name) + ": " + value;
    }
    
    private static String toJsonValue(String name, String value)
    {
        if (name == null) throw new NullArgumentException("name");
        if (value == null) throw new NullArgumentException("value");
        
        return javaString(name) + ": " + javaString(value);
    }
    
    private static String toJsonValue(String name, VolumeAccessPolicy value)
    {
        if (name == null) throw new NullArgumentException("name");
        if (value == null) throw new NullArgumentException("value");
        
        return javaString(name) + ": " + toJson(value);
    }
}

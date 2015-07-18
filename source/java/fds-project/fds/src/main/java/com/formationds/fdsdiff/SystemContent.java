package com.formationds.fdsdiff;

import static com.formationds.commons.util.Strings.javaString;

import java.io.Reader;
import java.time.Duration;
import java.time.Instant;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.function.Consumer;
import java.util.function.Function;
import java.util.stream.Collectors;

import com.formationds.client.ical.RecurrenceRule;
import com.formationds.client.v08.model.DataProtectionPolicy;
import com.formationds.client.v08.model.QosPolicy;
import com.formationds.client.v08.model.Size;
import com.formationds.client.v08.model.SnapshotPolicy;
import com.formationds.client.v08.model.Tenant;
import com.formationds.client.v08.model.User;
import com.formationds.client.v08.model.Volume;
import com.formationds.client.v08.model.VolumeAccessPolicy;
import com.formationds.client.v08.model.VolumeSettings;
import com.formationds.client.v08.model.VolumeStatus;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.model.ObjectManifest;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;

public class SystemContent
{
    public SystemContent()
    {
        _objectNames = new HashMap<>();
        _tenants = new HashSet<>();
        _users = new HashSet<>();
        _volumeNames = new HashMap<>();
        _volumes = new HashMap<>();
    }
    
    public final Set<String> getObjectNames(String volumeName)
    {
        if (volumeName == null) throw new NullArgumentException("volumeName");
        
        Volume volume = getVolume(volumeName);
        Set<ObjectManifest> volumeContent = getVolumeContent(volume);

        return volumeContent.stream()
                            .map(manifest -> manifest.getName())
                            .collect(Collectors.toSet());
    }
    
    public final Volume getVolume(String volumeName)
    {
        if (volumeName == null) throw new NullArgumentException("volumeName");
        
        Volume volume = _volumeNames.get(volumeName);
        if (volume == null)
        {
            throw new IllegalArgumentException(
                    "volume " + javaString(volumeName) + " does not exist.");
        }

        return volume;
    }
    
    public final Set<ObjectManifest> getVolumeContent(Volume volume)
    {
        if (volume == null) throw new NullArgumentException("volume");
        
        Set<ObjectManifest> volumeContent = _volumes.get(volume);
        if (volumeContent == null)
        {
            throw new IllegalArgumentException(
                    "volume " + javaString(volume.getName()) + " does not exist.");
        }
        
        return volumeContent;
    }
    
    public final Consumer<String> getVolumeObjectNameAdder(String volumeName)
    {
        if (volumeName == null) throw new NullArgumentException("volumeName");
        
        Consumer<ObjectManifest> volumeObjectAdder = getVolumeObjectAdder(volumeName);
        
        return objectName -> volumeObjectAdder.accept(new ObjectManifest.Builder<>()
                                                                        .setName(objectName)
                                                                        .build());
    }
    
    public final Consumer<ObjectManifest> getVolumeObjectAdder(String volumeName)
    {
        if (volumeName == null) throw new NullArgumentException("volumeName");
        
        Volume volume = getVolume(volumeName);
        Set<ObjectManifest> volumeContent = _volumes.get(volume);
        
        return manifest ->
        {
            String objectName = manifest.getName();
            ObjectManifest oldManifest = _objectNames.get(objectName);
            if (oldManifest != null)
            {
                volumeContent.remove(oldManifest);
            }
            volumeContent.add(manifest);
            _objectNames.put(objectName, manifest);
        };
    }
    
    public final Set<Volume> getVolumes()
    {
        return Collections.unmodifiableSet(_volumes.keySet());
    }
    
    public final void setTenants(Collection<Tenant> value)
    {
        if (value == null) throw new NullArgumentException("value");
        
        _tenants.clear();
        _tenants.addAll(value);
    }
    
    public final void setUsers(Collection<User> value)
    {
        if (value == null) throw new NullArgumentException("value");
        
        _users.clear();
        _users.addAll(value);
    }
    
    public final void setVolumes(Collection<Volume> value)
    {
        if (value == null) throw new NullArgumentException("value");
        
        _volumes.clear();
        for (Volume volume : value)
        {
            _volumes.put(volume, new HashSet<>());
            _volumeNames.put(volume.getName(), volume);
        }
    }
    
    @Override
    public final String toString()
    {
        return "{\n"
               + "  \"tenants\": [\n"
               + String.join(",\n", _tenants.stream()
                                            .map(t -> "    " + toJson(t))
                                            .<String>toArray(size -> new String[size])) + "\n"
               + "  ],\n"
               + "  \"users\": [\n"
               + String.join(",\n", _users.stream()
                                          .map(u -> "    " + toJson(u))
                                          .<String>toArray(size -> new String[size])) + "\n"
               + "  ],\n"
               + "  \"volumes\": [\n"
               + String.join(",\n", _volumes.keySet().stream()
                                            .map(v -> "    " + toJson(v, _volumes.get(v)))
                                            .<String>toArray(size -> new String[size])) + "\n"
               + "  ]\n"
               + "}\n";
    }
    
    public static SystemContent deserialize(Reader source)
    {
        JsonParser parser = new JsonParser();
        
        JsonElement json = parser.parse(source);
        
        JsonObject serialized = json.getAsJsonObject();
        
        serialized.getAsJsonArray("tenants");
    }
    
    private final Map<String, ObjectManifest> _objectNames;
    
    private final Set<Tenant> _tenants;

    private final Set<User> _users;

    private final Map<String, Volume> _volumeNames;

    private final Map<Volume, Set<ObjectManifest>> _volumes;
    
    private static <T> String toJson(Collection<T> collection, Function<T, String> elementConverter)
    {
        if (collection == null) throw new NullArgumentException("collection");
        if (elementConverter == null) throw new NullArgumentException("elementConverter");
        
        return "[ "
               + String.join(", ", collection.stream()
                                              .map(item -> elementConverter.apply(item))
                                              .<String>toArray(size -> new String[size]))
               +" ]";
    }
    
    private static <KeyT, ValueT> String toJson(Map<KeyT, ValueT> map,
                                                Function<KeyT, String> keyConverter,
                                                Function<ValueT, String> valueConverter)
    {
        if (map == null) throw new NullArgumentException("map");
        if (keyConverter == null) throw new NullArgumentException("keyConverter");
        if (valueConverter == null) throw new NullArgumentException("valueConverter");
        
        return "{ "
               + String.join(", ", map.entrySet()
                                      .stream()
                                      .map(entry -> keyConverter.apply(entry.getKey())
                                                    + ": "
                                                    + valueConverter.apply(entry.getValue()))
                                      .<String>toArray(size -> new String[size]))
               + " }";
    }
    
    private static String toJson(DataProtectionPolicy dataProtectionPolicy)
    {
        if (dataProtectionPolicy == null) throw new NullArgumentException("dataProtectionPolicy");
        
        return "{ "
               + toJsonValue("presetId", dataProtectionPolicy.getPresetId())
               + ", " + toJsonValue("commitLogRetention",
                                    dataProtectionPolicy.getCommitLogRetention())
               + ", " + toJsonValue("snapshotPolicies",
                                    dataProtectionPolicy.getSnapshotPolicies(),
                                    SystemContent::toJson)
               + " }";
    }
    
    private static String toJson(QosPolicy qosPolicy)
    {
        if (qosPolicy == null) throw new NullArgumentException("qosPolicy");
        
        return "{ "
               + toJsonValue("presetId", qosPolicy.getPresetID())
               + ", " + toJsonValue("priority", qosPolicy.getPriority())
               + ", " + toJsonValue("iopsMin", qosPolicy.getIopsMin())
               + ", " + toJsonValue("iopsMax", qosPolicy.getIopsMax())
               + " }";
    }
    
    private static String toJson(Size size)
    {
        if (size == null) throw new NullArgumentException("size");
        
        return "{ "
               + toJsonValue("value", size.getValue())
               + ", " + toJsonValue("unit", size.getUnit())
               + " }";
    }
    
    private static String toJson(SnapshotPolicy snapshotPolicy)
    {
        if (snapshotPolicy == null) throw new NullArgumentException("snapshotPolicy");
        
        return "{ "
               + toJsonValue("id", snapshotPolicy.getId())
               + ", " + toJsonValue("name", snapshotPolicy.getName())
               + ", " + toJsonValue("recurrenceRule", snapshotPolicy.getRecurrenceRule())
               + ", " + toJsonValue("retentionTime", snapshotPolicy.getRetentionTime())
               + ", " + toJsonValue("type", snapshotPolicy.getType())
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
               + (user.getTenant() == null
                  ? ""
                  : (", " + toJsonValue("tenant", user.getTenant().getId())))
               + ", " + toJsonValue("name", user.getName())
               + ", " + toJsonValue("roleId", user.getRoleId())
               + " }";
    }
    
    private static String toJson(Volume volume, Set<ObjectManifest> contents)
    {
        if (volume == null) throw new NullArgumentException("volume");
        
        return "{ "
               + toJsonValue("id", volume.getId())
               + ", " + toJsonValue("application", volume.getApplication())
               + ", " + toJsonValue("name", volume.getName())
               + ", " + toJsonValue("volumeAccessPolicy", volume.getAccessPolicy())
               + ", " + toJsonValue("created", volume.getCreated())
               + ", " + toJsonValue("dataProtectionPolicy", volume.getDataProtectionPolicy())
               + ", " + toJsonValue("mediaPolicy", volume.getMediaPolicy())
               + ", " + toJsonValue("qosPolicy", volume.getQosPolicy())
               + ", " + toJsonValue("settings", volume.getSettings())
               + ", " + toJsonValue("status", volume.getStatus())
               + ", " + toJsonValue("tags",
                                    volume.getTags(),
                                    k -> javaString(k),
                                    v -> javaString(v))
               + (volume.getTenant() == null
                  ? ""
                  : (", " + toJsonValue("tenant", volume.getTenant().getId())))
               + (contents == null
                  ? ""
                  : (", " + toJsonValue("objects", contents, v -> v.toJsonString())))
               + " }";
    }
    
    private static String toJson(VolumeAccessPolicy volumeAccessPolicy)
    {
        if (volumeAccessPolicy == null) throw new NullArgumentException("volumeAccessPolicy");
        
        return "{ "
               + toJsonValue("isExclusiveRead", volumeAccessPolicy.isExclusiveRead())
               + ", " + toJsonValue("isExclusiveWrite", volumeAccessPolicy.isExclusiveWrite())
               + " }";
    }
    
    private static String toJson(VolumeSettings volumeSettings)
    {
        if (volumeSettings == null) throw new NullArgumentException("volumeSettings");
        
        return "{ "
               + toJsonValue("volumeType", volumeSettings.getVolumeType())
               + " }";
    }
    
    private static String toJson(VolumeStatus volumeStatus)
    {
        if (volumeStatus == null) throw new NullArgumentException("volumeStatus");
        
        return "{ "
               + toJsonValue("lastCapacityFirebreak", volumeStatus.getLastCapacityFirebreak())
               + ", " + toJsonValue("lastPerformanceFirebreak",
                                    volumeStatus.getLastPerformanceFirebreak())
               + ", " + toJsonValue("currentUsage", volumeStatus.getCurrentUsage())
               + ", " + toJsonValue("state", volumeStatus.getState())
               + ", " + toJsonValue("timestamp", volumeStatus.getTimestamp())
               + " }";
    }
    
    private static String toJsonValue(String name, boolean value)
    {
        if (name == null) throw new NullArgumentException("name");
        
        return javaString(name) + ": " + value;
    }
    
    private static <T> String toJsonValue(String name,
                                          Collection<T> value,
                                          Function<T, String> elementConverter)
    {
        if (name == null) throw new NullArgumentException("name");
        if (value == null) throw new NullArgumentException("value");
        if (elementConverter == null) throw new NullArgumentException("elementConverter");

        return javaString(name) + ": " + toJson(value, elementConverter);
    }
    
    private static <KeyT, ValueT> String toJsonValue(String name,
                                                     Map<KeyT, ValueT> value,
                                                     Function<KeyT, String> keyConverter,
                                                     Function<ValueT, String> valueConverter)
    {
        if (name == null) throw new NullArgumentException("name");
        if (keyConverter == null) throw new NullArgumentException("keyConverter");
        if (valueConverter == null) throw new NullArgumentException("valueConverter");
        
        return javaString(name) + ": "
               + (value == null ? "null" : toJson(value, keyConverter, valueConverter));
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
    
    private static String toJsonValue(String name, Enum<?> value)
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
    
    private static String toJsonValue(String name, Number value)
    {
        if (name == null) throw new NullArgumentException("name");
        
        return javaString(name) + ": " + (value == null ? "null" : value);
    }
    
    private static String toJsonValue(String name, QosPolicy value)
    {
        if (name == null) throw new NullArgumentException("name");
        if (value == null) throw new NullArgumentException("value");
        
        return javaString(name) + ": " + toJson(value);
    }
    
    private static String toJsonValue(String name, RecurrenceRule value)
    {
        if (name == null) throw new NullArgumentException("name");
        if (value == null) throw new NullArgumentException("value");
        
        return javaString(name) + ": " + javaString(value.toString());
    }
    
    private static String toJsonValue(String name, Size value)
    {
        if (name == null) throw new NullArgumentException("name");
        if (value == null) throw new NullArgumentException("value");
        
        return javaString(name) + ": " + toJson(value);
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
    
    private static String toJsonValue(String name, VolumeSettings value)
    {
        if (name == null) throw new NullArgumentException("name");
        if (value == null) throw new NullArgumentException("value");
        
        return javaString(name) + ": " + toJson(value);
    }
    
    private static String toJsonValue(String name, VolumeStatus value)
    {
        if (name == null) throw new NullArgumentException("name");
        if (value == null) throw new NullArgumentException("value");
        
        return javaString(name) + ": " + toJson(value);
    }
}

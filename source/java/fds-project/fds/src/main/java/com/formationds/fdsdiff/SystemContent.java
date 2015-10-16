package com.formationds.fdsdiff;

import static com.formationds.commons.util.Strings.javaString;

import java.io.IOException;
import java.math.BigDecimal;
import java.text.ParseException;
import java.time.Duration;
import java.time.Instant;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Objects;
import java.util.Optional;
import java.util.Set;
import java.util.function.Consumer;
import java.util.stream.Collectors;

import com.formationds.client.ical.Numbers;
import com.formationds.client.ical.NumbersValidator;
import com.formationds.client.ical.RecurrenceRule;
import com.formationds.client.ical.WeekDays;
import com.formationds.client.ical.WeekDaysValidator;
import com.formationds.client.v08.model.DataProtectionPolicy;
import com.formationds.client.v08.model.MediaPolicy;
import com.formationds.client.v08.model.QosPolicy;
import com.formationds.client.v08.model.Size;
import com.formationds.client.v08.model.SizeUnit;
import com.formationds.client.v08.model.SnapshotPolicy;
import com.formationds.client.v08.model.SnapshotPolicy.SnapshotPolicyType;
import com.formationds.client.v08.model.Tenant;
import com.formationds.client.v08.model.User;
import com.formationds.client.v08.model.Volume;
import com.formationds.client.v08.model.VolumeAccessPolicy;
import com.formationds.client.v08.model.VolumeSettings;
import com.formationds.client.v08.model.VolumeSettingsBlock;
import com.formationds.client.v08.model.VolumeSettingsObject;
import com.formationds.client.v08.model.VolumeSettingsSystem;
import com.formationds.client.v08.model.VolumeState;
import com.formationds.client.v08.model.VolumeStatus;
import com.formationds.client.v08.model.VolumeType;
import com.formationds.commons.NullArgumentException;
import com.formationds.fdsdiff.SystemContent.GsonAdapter;
import com.formationds.iodriver.model.AbstractGsonAdapter;
import com.formationds.iodriver.model.BasicObjectManifest;
import com.formationds.iodriver.model.ComparisonDataFormat;
import com.formationds.iodriver.model.ExtendedObjectManifest;
import com.formationds.iodriver.model.FullObjectManifest;
import com.formationds.iodriver.model.ObjectManifest;
import com.google.gson.Gson;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.google.gson.annotations.JsonAdapter;
import com.google.gson.stream.JsonReader;
import com.google.gson.stream.JsonWriter;

@JsonAdapter(GsonAdapter.class)
public final class SystemContent
{
    public final static class GsonAdapter extends AbstractGsonAdapter<SystemContent, SystemContent>
    {
        @Override
        protected SystemContent build(SystemContent context)
        {
            if (context == null) throw new NullArgumentException("context");
            
            return context;
        }

        @Override
        protected SystemContent newContext()
        {
            return new SystemContent();
        }

        @Override
        protected void readValue(String name,
                                 JsonReader in,
                                 SystemContent context) throws IOException
        {
            if (name == null) throw new NullArgumentException("name");
            if (in == null) throw new NullArgumentException("in");
            if (context == null) throw new NullArgumentException("context");
            
            switch (name)
            {
            case "tenants":
                readNullable(in, reader -> { readTenants(reader, context); return null; });
                break;
            case "users":
                readNullable(in, reader -> { readUsers(reader, context); return null; });
                break;
            case "volumes":
                readNullable(in, reader -> { readVolumes(reader, context); return null; });
                break;
            default:
                throw new IOException("Unrecognized name: " + javaString(name) + ".");
            }
        }

        @Override
        protected void writeValues(SystemContent source, JsonWriter out) throws IOException
        {
            if (source == null) throw new NullArgumentException("source");
            if (out == null) throw new NullArgumentException("out");
            
            out.name("tenants");
            writeNullable(out, source._tenants, GsonAdapter::writeTenants);
            
            out.name("users");
            writeNullable(out, source._users, GsonAdapter::writeUsers);
            
            out.name("volumes");
            writeNullable(out, source._volumes, GsonAdapter::writeVolumes);
        }
        
        private static DataProtectionPolicy readDataProtectionPolicy(JsonReader in)
                throws IOException
        {
            if (in == null) throw new NullArgumentException("in");
            
            Long newPresetId = null;
            Duration newCommitLogRetention = null;
            List<SnapshotPolicy> newSnapshotPolicies = null;
            
            in.beginObject();
            while (in.hasNext())
            {
                String name = in.nextName();
                switch (name)
                {
                case "presetId":
                    newPresetId = readLongNullable(in);
                    break;
                case "commitLogRetention":
                    newCommitLogRetention = readDurationNullable(in);
                    break;
                case "snapshotPolicies":
                    newSnapshotPolicies = readNullable(
                            in,
                            reader -> readList(in, GsonAdapter::readSnapshotPolicy));
                    break;
                default:
                    throw new IOException("Name " + javaString(name) + " is unrecognized.");
                }
            }
            in.endObject();
            
            return new DataProtectionPolicy(newPresetId,
                                            newCommitLogRetention,
                                            newSnapshotPolicies);
        }
        
        private static Set<ObjectManifest> readObjects(JsonReader in) throws IOException
        {
            if (in == null) throw new NullArgumentException("in");
            
            Gson gson = new Gson();
            JsonParser parser = new JsonParser();
            Set<ObjectManifest> retval = new HashSet<>();
            
            in.beginArray();
            while (in.hasNext())
            {
                JsonElement json = parser.parse(in);
                JsonObject tree = json.getAsJsonObject();
                
                Class<? extends ObjectManifest> manifestClass = null;
                switch (ComparisonDataFormat.valueOf(tree.get("type").getAsString()))
                {
                case MINIMAL: manifestClass = ObjectManifest.class; break;
                case BASIC: manifestClass = BasicObjectManifest.class; break;
                case EXTENDED: manifestClass = ExtendedObjectManifest.class; break;
                case FULL: manifestClass = FullObjectManifest.class; break;
                }

                retval.add(gson.fromJson(tree.get("value"), manifestClass));
            }
            in.endArray();
            
            return retval;
        }
        
        private static QosPolicy readQosPolicy(JsonReader in) throws IOException
        {
            if (in == null) throw new NullArgumentException("in");
            
            Long newPresetId = null;
            Integer newPriority = null;
            Integer newIopsMin = null;
            Integer newIopsMax = null;
            
            in.beginObject();
            while (in.hasNext())
            {
                String name = in.nextName();
                switch (name)
                {
                case "presetId": newPresetId = readLongNullable(in); break;
                case "priority": newPriority = readIntegerNullable(in); break;
                case "iopsMin": newIopsMin = readIntegerNullable(in); break;
                case "iopsMax": newIopsMax = readIntegerNullable(in); break;
                default: throw new IOException("Unrecognized name: " + javaString(name) + ".");
                }
            }
            in.endObject();
            
            return new QosPolicy(newPresetId, newPriority, newIopsMin, newIopsMax);
        }
        
        private static RecurrenceRule readRecurrenceRule(JsonReader in) throws IOException
        {
            if (in == null) throw new NullArgumentException("in");
            
            String newRecurrenceRuleString = in.nextString();
            try
            {
                return new RecurrenceRule().parser(newRecurrenceRuleString);
            }
            catch (ParseException e)
            {
                throw new IOException("Error parsing " + RecurrenceRule.class.getName() + ": "
                        + javaString(newRecurrenceRuleString)
                        + ".");
            }
        }
        
        private static Size readSize(JsonReader in) throws IOException
        {
            if (in == null) throw new NullArgumentException("in");
            
            SizeUnit newUnit = null;
            BigDecimal newValue = null;
            
            in.beginObject();
            while (in.hasNext())
            {
                String name = in.nextName();
                switch (name)
                {
                case "unit":
                    newUnit = readNullable(in, reader -> SizeUnit.valueOf(in.nextString()));
                    break;
                case "value":
                    newValue = readNullable(in, reader -> new BigDecimal(in.nextString()));
                    break;
                default:
                    throw new IOException("Name " + javaString(name) + " is unrecognized.");
                }
            }
            in.endObject();
            
            return new Size(newValue, newUnit);
        }
        
        private static SnapshotPolicy readSnapshotPolicy(JsonReader in) throws IOException
        {
            if (in == null) throw new NullArgumentException("in");
            
            Long newId = null;
            String newName = null;
            RecurrenceRule newRecurrenceRule = null;
            Duration newRetentionTime = null;
            SnapshotPolicyType newSnapshotPolicyType = null;
            
            in.beginObject();
            while (in.hasNext())
            {
                String name = in.nextName();
                switch (name)
                {
                case "id":
                    newId = readLongNullable(in);
                    break;
                case "name":
                    newName = in.nextString();
                    break;
                case "recurrenceRule":
                    newRecurrenceRule = readNullable(in, GsonAdapter::readRecurrenceRule);
                    break;
                case "retentionTime":
                    newRetentionTime = readDurationNullable(in);
                    break;
                case "type":
                    newSnapshotPolicyType =
                            readNullable(in, reader -> SnapshotPolicyType.valueOf(in.nextString()));
                    break;
                default:
                    throw new IOException("Unrecognized name: " + javaString(name) + ".");
                }
            }
            in.endObject();
            
            return new SnapshotPolicy(newId,
                                      newName,
                                      newSnapshotPolicyType,
                                      newRecurrenceRule,
                                      newRetentionTime);
        }
        
        private static Tenant readTenant(JsonReader in) throws IOException
        {
            if (in == null) throw new NullArgumentException("in");
            
            Long newId = null;
            String newName = null;
            
            in.beginObject();
            while (in.hasNext())
            {
                String name = in.nextName();
                switch (name)
                {
                case "id": newId = readLongNullable(in); break;
                case "name": newName = in.nextString(); break;
                default: throw new IOException("Unrecognized name: " + javaString(name) + ".");
                }
            }
            in.endObject();
            
            return new Tenant(newId, newName);
        }
        
        private static void readTenants(JsonReader in, SystemContent context) throws IOException
        {
            if (in == null) throw new NullArgumentException("in");
            if (context == null) throw new NullArgumentException("context");
            
            in.beginArray();
            while (in.hasNext())
            {
                context.addTenant(readTenant(in));
            }
            in.endArray();
        }
        
        // NOTE: The User.tenant is an ID-only placeholder.
        private static User readUser(JsonReader in) throws IOException
        {
            if (in == null) throw new NullArgumentException("in");
            
            Long newId = null;
            String newName = null;
            Long newRoleId = null;
            Long newTenantId = null;
            
            in.beginObject();
            while (in.hasNext())
            {
                String name = in.nextName();
                switch (name)
                {
                case "id": newId = readLongNullable(in); break;
                case "tenantId": newTenantId = readLongNullable(in); break;
                case "name": newName = in.nextString(); break;
                case "roleId": newRoleId = readLongNullable(in); break;
                default: throw new IOException("Unrecognized name: " + javaString(name) + ".");
                }
            }
            in.endObject();
            
            return new User(newId, newName, newRoleId, newTenantId == null
                                                       ? null
                                                       : new Tenant(newTenantId, null));
        }
        
        private static void readUsers(JsonReader in, SystemContent context) throws IOException
        {
            if (in == null) throw new NullArgumentException("in");
            if (context == null) throw new NullArgumentException("context");
            
            in.beginArray();
            while (in.hasNext())
            {
                context.addUser(readUser(in));
            }
            in.endArray();
        }
        
        private static void readVolume(JsonReader in, SystemContent context) throws IOException
        {
            if (in == null) throw new NullArgumentException("in");
            if (context == null) throw new NullArgumentException("context");

            Long newId = null;
            String newApplication = null;
            String newName = null;
            VolumeAccessPolicy newAccessPolicy = null;
            Instant newCreated = null;
            DataProtectionPolicy newDataProtectionPolicy = null;
            MediaPolicy newMediaPolicy = null;
            QosPolicy newQosPolicy = null;
            VolumeSettings newSettings = null;
            VolumeStatus newStatus = null;
            Map<String, String> newTags = null;
            Long newTenantId = null;
            Set<ObjectManifest> newObjects = null;
            
            in.beginObject();
            while (in.hasNext())
            {
                String name = in.nextName();
                switch (name)
                {
                case "id":
                    newId = readLongNullable(in);
                    break;
                case "application":
                    newApplication = in.nextString();
                    break;
                case "name":
                    newName = in.nextString();
                    break;
                case "accessPolicy":
                    newAccessPolicy = readNullable(in, GsonAdapter::readVolumeAccessPolicy);
                    break;
                case "created":
                    newCreated = readInstantNullable(in);
                    break;
                case "dataProtectionPolicy":
                    newDataProtectionPolicy = readNullable(in,
                                                           GsonAdapter::readDataProtectionPolicy);
                    break;
                case "mediaPolicy":
                    newMediaPolicy =
                            readNullable(in,
                                         reader -> MediaPolicy.valueOf(reader.nextString()));
                    break;
                case "qosPolicy":
                    newQosPolicy = readNullable(in, GsonAdapter::readQosPolicy);
                    break;
                case "settings":
                    newSettings = readNullable(in, GsonAdapter::readVolumeSettings);
                    break;
                case "status":
                    newStatus = readNullable(in, GsonAdapter::readVolumeStatus);
                    break;
                case "tags":
                    newTags = readMapNullable(in, GsonAdapter::readMapStringEntry);
                    break;
                case "tenant":
                    newTenantId = readLongNullable(in);
                    break;
                case "objects":
                    newObjects = readNullable(in, GsonAdapter::readObjects);
                    break;
                default:
                    throw new IOException("Unrecognized name: " + javaString(name) + ".");
                }
            }
            in.endObject();
            
            Volume newVolume = new Volume(newId,
                                          newName,
                                          newTenantId == null ? null : new Tenant(newTenantId, null),
                                          newApplication,
                                          newStatus,
                                          newSettings,
                                          newMediaPolicy,
                                          newDataProtectionPolicy,
                                          newAccessPolicy,
                                          newQosPolicy,
                                          newCreated,
                                          newTags);
            VolumeWrapper newVolumeWrapper = new VolumeWrapper(newVolume);
            context.addVolume(newVolumeWrapper);
            newObjects.forEach(context.getVolumeObjectAdder(newVolumeWrapper));
        }
        
        private static void readVolumes(JsonReader in, SystemContent context) throws IOException
        {
            if (in == null) throw new NullArgumentException("in");
            if (context == null) throw new NullArgumentException("context");
            
            in.beginArray();
            while (in.hasNext())
            {
                readVolume(in, context);
            }
            in.endArray();
        }
        
        private static VolumeAccessPolicy readVolumeAccessPolicy(JsonReader in) throws IOException
        {
            if (in == null) throw new NullArgumentException("in");
            
            Boolean newIsExclusiveRead = null;
            Boolean newIsExclusiveWrite = null;
            
            in.beginObject();
            while (in.hasNext())
            {
                String name = in.nextName();
                switch (name)
                {
                case "isExclusiveRead":
                    newIsExclusiveRead = readBooleanNullable(in);
                    break;
                case "isExclusiveWrite":
                    newIsExclusiveWrite = readBooleanNullable(in);
                    break;
                default:
                    throw new IOException("Name " + javaString(name) + " is unrecognized.");
                }
            }
            in.endObject();
            
            return new VolumeAccessPolicy(newIsExclusiveRead, newIsExclusiveWrite);
        }
        
        private static VolumeSettings readVolumeSettings(JsonReader in) throws IOException
        {
            if (in == null) throw new NullArgumentException("in");
            
            VolumeType newVolumeType = null;
            Size newBlockSize = null;
            Size newCapacity = null;
            Size newMaxObjectSize = null;
            
            in.beginObject();
            while (in.hasNext())
            {
                String name = in.nextName();
                switch (name)
                {
                case "volumeType":
                    newVolumeType = readNullable(in,
                                                 reader -> VolumeType.valueOf(reader.nextString()));
                    break;
                case "blockSize":
                    newBlockSize = readNullable(in, GsonAdapter::readSize);
                    break;
                case "capacity":
                    newCapacity = readNullable(in, GsonAdapter::readSize);
                    break;
                case "maxObjectSize":
                    newMaxObjectSize = readNullable(in, GsonAdapter::readSize);
                    break;
                default:
                    throw new IOException("Name " + javaString(name) + " is unrecognized.");
                }
            }
            in.endObject();
            
            switch (newVolumeType)
            {
            case BLOCK: return new VolumeSettingsBlock(newCapacity, newBlockSize);
            case OBJECT: return new VolumeSettingsObject(newMaxObjectSize);
            case SYSTEM: return new VolumeSettingsSystem();
            default: throw new IOException("Unrecognized " + VolumeSettings.class.getName()
                                           + " type: " + newVolumeType + ".");
            }
        }
        
        private static VolumeStatus readVolumeStatus(JsonReader in) throws IOException
        {
            if (in == null) throw new NullArgumentException("in");
            
            Instant newLastCapacityFirebreak = null;
            Instant newLastPerformanceFirebreak = null;
            Size newCurrentUsage = null;
            VolumeState newState = null;
            Instant newTimestamp = null;
            
            in.beginObject();
            while (in.hasNext())
            {
                String name = in.nextName();
                switch (name)
                {
                case "lastCapacityFirebreak":
                    newLastCapacityFirebreak = readInstantNullable(in);
                    break;
                case "lastPerformanceFirebreak":
                    newLastPerformanceFirebreak = readInstantNullable(in);
                    break;
                case "currentUsage":
                    newCurrentUsage = readNullable(in, GsonAdapter::readSize);
                    break;
                case "state":
                    newState = readNullable(in, reader -> VolumeState.valueOf(reader.nextString()));
                    break;
                case "timestamp":
                    newTimestamp = readInstantNullable(in);
                    break;
                default:
                    throw new IOException("Name " + javaString(name) + " is unrecognized.");
                }
            }
            in.endObject();
            
            return new VolumeStatus(newTimestamp,
                                    newState,
                                    newCurrentUsage,
                                    newLastCapacityFirebreak,
                                    newLastPerformanceFirebreak);
        }
        
        private static void writeDataProtectionPolicy(JsonWriter out,
                                                      DataProtectionPolicy value) throws IOException
        {
            if (out == null) throw new NullArgumentException("out");
            if (value == null) throw new NullArgumentException("value");

            out.beginObject();
            out.name("presetId");
            out.value(value.getPresetId());
            out.name("commitLogRetention");
            writeNullableDuration(out, value.getCommitLogRetention());
            out.name("snapshotPolicies");
            writeNullable(out, value.getSnapshotPolicies(), GsonAdapter::writeSnapshotPolicies);
            out.endObject();
        }
        
        private static void writeObjectManifests(JsonWriter out,
                                                 Set<ObjectManifest> value) throws IOException
        {
            if (out == null) throw new NullArgumentException("out");
            if (value == null) throw new NullArgumentException("value");
            
            Gson gson = new Gson();
            
            out.beginArray();
            for (ObjectManifest objectManifest : value)
            {
                out.beginObject();
                out.name("type");
                out.value(objectManifest.getFormat().toString());
                out.name("value");
                gson.toJson(objectManifest, objectManifest.getClass(), out);
                out.endObject();
            }
            out.endArray();
        }
        
        private static void writeQosPolicy(JsonWriter out, QosPolicy value) throws IOException
        {
            if (out == null) throw new NullArgumentException("out");
            if (value == null) throw new NullArgumentException("value");
            
            out.beginObject();
            out.name("presetId");
            out.value(value.getPresetID());
            out.name("priority");
            out.value(value.getPriority());
            out.name("iopsMin");
            out.value(value.getIopsMin());
            out.name("iopsMax");
            out.value(value.getIopsMax());
            out.endObject();
        }
        
        private static void writeSize(JsonWriter out, Size value) throws IOException
        {
            if (out == null) throw new NullArgumentException("out");
            if (value == null) throw new NullArgumentException("value");
            
            out.beginObject();
            out.name("unit");
            out.value(value.getUnit().toString());
            out.name("value");
            out.value(value.getValue());
            out.endObject();
        }
        
        private static void writeSnapshotPolicy(JsonWriter out,
                                                SnapshotPolicy value) throws IOException
        {
            if (out == null) throw new NullArgumentException("out");
            if (value == null) throw new NullArgumentException("value");

            out.beginObject();
            out.name("id");
            out.value(value.getId());
            out.name("name");
            out.value(value.getName());
            out.name("recurrenceRule");
            writeNullable(out, value.getRecurrenceRule(), (w, v) -> w.value(v.toString()));
            out.name("retentionTime");
            writeNullableDuration(out, value.getRetentionTime());
            out.name("type");
            writeNullable(out, value.getType(), (w, v) -> w.value(v.toString()));
            out.endObject();
        }
        
        private static void writeSnapshotPolicies(JsonWriter out,
                                                  List<SnapshotPolicy> value) throws IOException
        {
            if (out == null) throw new NullArgumentException("out");
            if (value == null) throw new NullArgumentException("value");

            out.beginArray();
            for (SnapshotPolicy snapshotPolicy : value)
            {
                writeSnapshotPolicy(out, snapshotPolicy);
            }
            out.endArray();
        }
        
        private static void writeTenant(JsonWriter out, Tenant value) throws IOException
        {
            if (out == null) throw new NullArgumentException("out");
            if (value == null) throw new NullArgumentException("value");

            out.beginObject();
            out.name("id");
            out.value(value.getId());
            out.name("name");
            out.value(value.getName());
            out.endObject();
        }
        
        private static void writeTenants(JsonWriter out, Set<Tenant> value) throws IOException
        {
            if (out == null) throw new NullArgumentException("out");
            if (value == null) throw new NullArgumentException("value");
            
            out.beginArray();
            for (Tenant tenant : value)
            {
                writeTenant(out, tenant);
            }
            out.endArray();
        }
        
        private static void writeUser(JsonWriter out, User value) throws IOException
        {
            if (out == null) throw new NullArgumentException("out");
            if (value == null) throw new NullArgumentException("value");
            
            out.beginObject();
            out.name("id");
            out.value(value.getId());
            out.name("tenantId");
            out.value(Optional.ofNullable(value.getTenant()).map(t -> t.getId()).orElse(null));
            out.name("name");
            out.value(value.getName());
            out.name("roleId");
            out.value(value.getRoleId());
            out.endObject();
        }
        
        private static void writeUsers(JsonWriter out, Set<User> value) throws IOException
        {
            if (out == null) throw new NullArgumentException("out");
            if (value == null) throw new NullArgumentException("value");
            
            out.beginArray();
            for (User user : value)
            {
                writeUser(out, user);
            }
            out.endArray();
        }
        
        private static void writeVolume(JsonWriter out,
                                        Entry<VolumeWrapper,
                                              Set<ObjectManifest>> value) throws IOException
        {
            if (out == null) throw new NullArgumentException("out");
            if (value == null) throw new NullArgumentException("value");

            Volume volume = value.getKey().getVolume();
            Set<ObjectManifest> objects = value.getValue();
            
            out.beginObject();
            out.name("id");
            out.value(volume.getId());
            out.name("application");
            out.value(volume.getApplication());
            out.name("name");
            out.value(volume.getName());
            out.name("accessPolicy");
            writeNullable(out, volume.getAccessPolicy(), GsonAdapter::writeVolumeAccessPolicy);
            out.name("created");
            writeNullableInstant(out, volume.getCreated());
            out.name("dataProtectionPolicy");
            writeNullable(out,
                          volume.getDataProtectionPolicy(),
                          GsonAdapter::writeDataProtectionPolicy);
            out.name("mediaPolicy");
            writeNullable(out, volume.getMediaPolicy(), (w, v) -> w.value(v.toString()));
            out.name("qosPolicy");
            writeNullable(out, volume.getQosPolicy(), GsonAdapter::writeQosPolicy);
            out.name("settings");
            writeNullable(out, volume.getSettings(), GsonAdapter::writeVolumeSettings);
            out.name("status");
            writeNullable(out, volume.getStatus(), GsonAdapter::writeVolumeStatus);
            out.name("tags");
            writeNullableMap(out, volume.getTags(), GsonAdapter::writeMapStringEntry);
            out.name("tenant");
            writeNullable(out, volume.getTenant(), (w, v) -> w.value(v.getId()));
            out.name("objects");
            writeObjectManifests(out, objects); 
            out.endObject();
        }
        
        private static void writeVolumeAccessPolicy(JsonWriter out,
                                                    VolumeAccessPolicy value) throws IOException
        {
            if (out == null) throw new NullArgumentException("out");
            if (value == null) throw new NullArgumentException("value");
            
            out.beginObject();
            out.name("isExclusiveRead");
            writeNullableBoolean(out, value.isExclusiveRead());
            out.name("isExclusiveWrite");
            writeNullableBoolean(out, value.isExclusiveWrite());
            out.endObject();
        }
        
        private static void writeVolumeSettings(JsonWriter out,
                                                VolumeSettings value) throws IOException
        {
            if (out == null) throw new NullArgumentException("out");
            if (value == null) throw new NullArgumentException("value");
            
            VolumeType volumeType = value.getVolumeType();

            out.beginObject();
            out.name("volumeType");
            writeNullable(out, volumeType, (w, v) -> w.value(v.toString()));
            switch (volumeType)
            {
            case BLOCK:
            {
                VolumeSettingsBlock typedValue = (VolumeSettingsBlock)value;
                out.name("blockSize");
                writeNullable(out, typedValue.getBlockSize(), GsonAdapter::writeSize);
                out.name("capacity");
                writeNullable(out, typedValue.getCapacity(), GsonAdapter::writeSize);
                break;
            }
            case OBJECT:
            {
                VolumeSettingsObject typedValue = (VolumeSettingsObject)value;
                out.name("maxObjectSize");
                writeNullable(out, typedValue.getMaxObjectSize(), GsonAdapter::writeSize);
            }
            case SYSTEM:
                break;
            default:
                throw new IOException("Unrecognized " + VolumeType.class.getName() + ": "
                                      + String.valueOf(volumeType) + ".");
            }
            out.endObject();
        }
        
        private static void writeVolumeStatus(JsonWriter out,
                                              VolumeStatus value) throws IOException
        {
            if (out == null) throw new NullArgumentException("out");
            if (value == null) throw new NullArgumentException("value");
            
            out.beginObject();
            out.name("lastCapacityFirebreak");
            writeNullableInstant(out, value.getLastCapacityFirebreak());
            out.name("lastPerformanceFirebreak");
            writeNullableInstant(out, value.getLastPerformanceFirebreak());
            out.name("currentUsage");
            writeNullable(out, value.getCurrentUsage(), GsonAdapter::writeSize);
            out.name("state");
            writeNullable(out, value.getState(), (w, v) -> w.value(v.toString()));
            out.name("timestamp");
            writeNullableInstant(out, value.getTimestamp());
            out.endObject();
        }

        private static void writeVolumes(JsonWriter out,
                                         Map<VolumeWrapper,
                                             Set<ObjectManifest>> value) throws IOException
        {
            if (out == null) throw new NullArgumentException("out");
            if (value == null) throw new NullArgumentException("value");

            out.beginArray();
            for (Entry<VolumeWrapper, Set<ObjectManifest>> entry : value.entrySet())
            {
                writeVolume(out, entry);
            }
            out.endArray();
        }
    }
    
    public final static class VolumeWrapper
    {
        public VolumeWrapper(Volume volume)
        {
            if (volume == null) throw new NullArgumentException("volume");
            
            _volume = volume;
        }
        
        @Override
        public boolean equals(Object other)
        {
            if (other == null || other.getClass() != getClass())
            {
                return false;
            }
            else
            {
                VolumeWrapper typedOther = (VolumeWrapper)other;
                Volume otherVolume = typedOther._volume;
                
                return _equals(_volume.getAccessPolicy(), otherVolume.getAccessPolicy())
                       && Objects.equals(_volume.getApplication(), otherVolume.getApplication())
                       && Objects.equals(_volume.getCreated(), otherVolume.getCreated())
                       && _equals(_volume.getDataProtectionPolicy(),
                                  otherVolume.getDataProtectionPolicy())
                       && Objects.equals(_volume.getId(), otherVolume.getId())
                       && Objects.equals(_volume.getMediaPolicy(), otherVolume.getMediaPolicy())
                       && Objects.equals(_volume.getName(), otherVolume.getName())
                       && _equals(_volume.getQosPolicy(), otherVolume.getQosPolicy())
                       && _equals(_volume.getSettings(), otherVolume.getSettings())
                       && _equals(_volume.getStatus(), otherVolume.getStatus())
                       && Objects.equals(_volume.getTags(), otherVolume.getTags())
                       && _equals(_volume.getTenant(), otherVolume.getTenant());
            }
        }
        
        public Volume getVolume()
        {
            return _volume;
        }
        
        @Override
        public int hashCode()
        {
            int hashCode = 0xABadCab5;
            
            hashCode = hashCode * 23 ^ _hashCode(_volume.getAccessPolicy());
            hashCode = hashCode * 23 ^ Objects.hashCode(_volume.getApplication());
            hashCode = hashCode * 23 ^ Objects.hashCode(_volume.getCreated());
            hashCode = hashCode * 23 ^ _hashCode(_volume.getDataProtectionPolicy());
            hashCode = hashCode * 23 ^ Objects.hashCode(_volume.getId());
            hashCode = hashCode * 23 ^ Objects.hashCode(_volume.getMediaPolicy());
            hashCode = hashCode * 23 ^ Objects.hashCode(_volume.getName());
            hashCode = hashCode * 23 ^ _hashCode(_volume.getQosPolicy());
            hashCode = hashCode * 23 ^ _hashCode(_volume.getSettings());
            hashCode = hashCode * 23 ^ _hashCode(_volume.getStatus());
            hashCode = hashCode * 23 ^ Objects.hashCode(_volume.getTags());
            hashCode = hashCode * 23 ^ _hashCode(_volume.getTenant());
            
            return hashCode;
        }
        
        private final Volume _volume;
        
        private static boolean _equals(DataProtectionPolicy a, DataProtectionPolicy b)
        {
            if (a == b)
            {
                return true;
            }
            else if (a == null
                     || b == null
                     || a.getClass() != DataProtectionPolicy.class
                     || b.getClass() != DataProtectionPolicy.class)
            {
                return false;
            }
            else
            {
                return Objects.equals(a.getCommitLogRetention(), b.getCommitLogRetention())
                       && Objects.equals(a.getPresetId(), b.getPresetId())
                       && _equals(a.getSnapshotPolicies(), b.getSnapshotPolicies());
            }
        }
        
        private static boolean _equals(List<SnapshotPolicy> a, List<SnapshotPolicy> b)
        {
            if (a == b)
            {
                return true;
            }
            else if (a == null || b == null || a.size() != b.size())
            {
                return false;
            }
            else
            {
                for (int i = 0; i != a.size(); i++)
                {
                    if (!_equals(a.get(i), b.get(i)))
                    {
                        return false;
                    }
                }
                return true;
            }
        }
        
        private static boolean _equals(Numbers<Integer> a, Numbers<Integer> b)
        {
            if (a == b)
            {
                return true;
            }
            else if (a == null
                     || b == null
                     || a.getClass() != Numbers.class
                     || b.getClass() != Numbers.class)
            {
                return false;
            }
            else
            {
                return a.equals(b)
                       && _equals(a.validator(), b.validator());
            }
        }
        
        private static boolean _equals(NumbersValidator<Integer> a, NumbersValidator<Integer> b)
        {
            if (a == b)
            {
                return true;
            }
            else if (a == null
                     || b == null
                     || a.getClass() != NumbersValidator.class
                     || b.getClass() != NumbersValidator.class)
            {
                return false;
            }
            else
            {
                return true;
            }
        }
        
        private static boolean _equals(QosPolicy a, QosPolicy b)
        {
            if (a == b)
            {
                return true;
            }
            else if (a == null
                     || b == null
                     || a.getClass() != QosPolicy.class
                     || b.getClass() != QosPolicy.class)
            {
                return false;
            }
            else
            {
                return Objects.equals(a.getIopsMax(), b.getIopsMax())
                       && Objects.equals(a.getIopsMin(), b.getIopsMin())
                       && Objects.equals(a.getPresetID(), b.getPresetID())
                       && Objects.equals(a.getPriority(), b.getPriority());
            }
        }
        
        private static boolean _equals(RecurrenceRule a, RecurrenceRule b)
        {
            if (a == b)
            {
                return true;
            }
            else if (a == null
                     || b == null
                     || a.getClass() != RecurrenceRule.class
                     || b.getClass() != RecurrenceRule.class)
            {
                return false;
            }
            else
            {
                return Objects.equals(a.getCount(), b.getCount())
                       && _equals((WeekDays<?>)a.getDays(), (WeekDays<?>)b.getDays())
                       && Objects.equals(a.getFrequency(), b.getFrequency())
                       && _equals(a.getHours(), b.getHours())
                       && Objects.equals(a.getInterval(), b.getInterval())
                       && _equals(a.getMinutes(), b.getMinutes())
                       && _equals(a.getMonthDays(), b.getMonthDays())
                       && _equals(a.getMonths(), b.getMonths())
                       && _equals(a.getPosition(), b.getPosition())
                       && _equals(a.getSeconds(), b.getSeconds())
                       && _equals(a.getWeekNo(), b.getWeekNo())
                       && Objects.equals(a.getUntil(), b.getUntil())
                       && Objects.equals(a.getWeekStartDay(), b.getWeekStartDay())
                       && _equals(a.getYearDays(), b.getYearDays());
            }
        }
        
        private static boolean _equals(Size a, Size b)
        {
            if (a == b)
            {
                return true;
            }
            else if (a == null
                     || b == null
                     || a.getClass() != Size.class
                     || b.getClass() != Size.class)
            {
                return false;
            }
            else
            {
                return Objects.equals(a.getUnit(), b.getUnit())
                       && Objects.equals(a.getValue(), b.getValue());
            }
        }
        
        private static boolean _equals(SnapshotPolicy a, SnapshotPolicy b)
        {
            if (a == b)
            {
                return true;
            }
            else if (a == null
                     || b == null
                     || a.getClass() != SnapshotPolicy.class
                     || b.getClass() != SnapshotPolicy.class)
            {
                return false;
            }
            else
            {
                return Objects.equals(a.getId(), b.getId())
                       && Objects.equals(a.getName(), b.getName())
                       && _equals(a.getRecurrenceRule(), b.getRecurrenceRule())
                       && Objects.equals(a.getRetentionTime(), b.getRetentionTime())
                       && Objects.equals(a.getType(), b.getType());
            }
        }
        
        private static boolean _equals(Tenant a, Tenant b)
        {
            if (a == b)
            {
                return true;
            }
            else if (a == null
                     || b == null
                     || a.getClass() != Tenant.class
                     || b.getClass() != Tenant.class)
            {
                return false;
            }
            else
            {
                return Objects.equals(a.getId(), b.getId())
                       && Objects.equals(a.getName(), b.getName());
            }
        }
        
        private static boolean _equals(VolumeAccessPolicy a, VolumeAccessPolicy b)
        {
            if (a == b)
            {
                return true;
            }
            else if (a == null
                     || b == null
                     || a.getClass() != VolumeAccessPolicy.class
                     || b.getClass() != VolumeAccessPolicy.class)
            {
                return false;
            }
            else
            {
                return a.isExclusiveRead() == b.isExclusiveRead()
                       && a.isExclusiveWrite() == b.isExclusiveWrite();
            }
        }
        
        private static boolean _equals(VolumeSettings a, VolumeSettings b)
        {
            if (a == b)
            {
                return true;
            }
            else if (a == null
                     || b == null
                     || a.getClass() != b.getClass())
            {
                return false;
            }
            else
            {
                if (a.getClass() == VolumeSettingsBlock.class)
                {
                    return _equals((VolumeSettingsBlock)a, (VolumeSettingsBlock)b);
                }
                else if (a.getClass() == VolumeSettingsObject.class)
                {
                    return _equals((VolumeSettingsObject)a, (VolumeSettingsObject)b);
                }
                else if (a.getClass() == VolumeSettingsSystem.class)
                {
                    return Objects.equals(a.getVolumeType(), b.getVolumeType());
                }
                else
                {
                    return false;
                }
            }
        }
        
        private static boolean _equals(VolumeSettingsBlock a, VolumeSettingsBlock b)
        {
            if (a == b)
            {
                return true;
            }
            else if (a == null
                     || b == null
                     || a.getClass() != VolumeSettingsBlock.class
                     || b.getClass() != VolumeSettingsBlock.class)
            {
                return false;
            }
            else
            {
                return Objects.equals(a.getVolumeType(), b.getVolumeType())
                       && _equals(a.getBlockSize(), b.getBlockSize())
                       && _equals(a.getCapacity(), b.getCapacity());
            }
        }
        
        private static boolean _equals(VolumeSettingsObject a, VolumeSettingsObject b)
        {
            if (a == b)
            {
                return true;
            }
            else if (a == null
                     || b == null
                     || a.getClass() != VolumeSettingsObject.class
                     || b.getClass() != VolumeSettingsObject.class)
            {
                return false;
            }
            else
            {
                return Objects.equals(a.getVolumeType(), b.getVolumeType())
                       && _equals(a.getMaxObjectSize(), b.getMaxObjectSize());
            }
        }
        
        private static boolean _equals(VolumeStatus a, VolumeStatus b)
        {
            if (a == b)
            {
                return true;
            }
            else if (a == null
                     || b == null
                     || a.getClass() != VolumeStatus.class
                     || b.getClass() != VolumeStatus.class)
            {
                return false;
            }
            else
            {
                return _equals(a.getCurrentUsage(), b.getCurrentUsage())
                       && Objects.equals(a.getLastCapacityFirebreak(), b.getLastCapacityFirebreak())
                       && Objects.equals(a.getLastPerformanceFirebreak(),
                                         b.getLastPerformanceFirebreak())
                       && Objects.equals(a.getState(), b.getState())
                       && Objects.equals(a.getTimestamp(), b.getTimestamp());
            }
        }
        
        private static boolean _equals(WeekDays<?> a, WeekDays<?> b)
        {
            if (a == b)
            {
                return true;
            }
            else if (a == null
                     || b == null
                     || a.getClass() != WeekDays.class
                     || b.getClass() != WeekDays.class)
            {
                return false;
            }
            else
            {
                return a.equals(b)
                       && _equals(a.validator(), b.validator());
            }
        }
        
        private static boolean _equals(WeekDaysValidator<?> a, WeekDaysValidator<?> b)
        {
            if (a == b)
            {
                return true;
            }
            else if (a == null
                     || b == null
                     || a.getClass() != WeekDaysValidator.class
                     || b.getClass() != WeekDaysValidator.class)
            {
                return false;
            }
            else
            {
                return true;
            }
        }
        
        private static int _hashCode(DataProtectionPolicy value)
        {
            if (value == null)
            {
                return 0;
            }
            else
            {
                int hashCode = 0xABadCab5;
                
                hashCode = hashCode * 23 ^ Objects.hashCode(value.getCommitLogRetention());
                hashCode = hashCode * 23 ^ Objects.hashCode(value.getPresetId());
                hashCode = hashCode * 23 ^ _hashCode(value.getSnapshotPolicies());
                
                return hashCode;
            }
        }
        
        private static int _hashCode(List<SnapshotPolicy> value)
        {
            if (value == null)
            {
                return 0;
            }
            else
            {
                int hashCode = 0xABadCab5;
                
                for (SnapshotPolicy element : value)
                {
                    hashCode = hashCode * 23 ^ _hashCode(element);
                }
                
                return hashCode;
            }
        }
        
        private static int _hashCode(Numbers<Integer> value)
        {
            if (value == null)
            {
                return 0;
            }
            else
            {
                int hashCode = 0xABadCab5;
                
                hashCode = hashCode * 23 ^ Objects.hashCode(value);
                hashCode = hashCode * 23 ^ _hashCode(value.validator());
                
                return hashCode;
            }
        }
        
        private static int _hashCode(NumbersValidator<Integer> value)
        {
            if (value == null)
            {
                return 0;
            }
            else
            {
                int hashCode = 0xABadCab5;
                
                hashCode = hashCode * 23 ^ Objects.hashCode(value.getClass());
                
                return hashCode;
            }
        }
        
        private static int _hashCode(QosPolicy value)
        {
            if (value == null)
            {
                return 0;
            }
            else
            {
                int hashCode = 0xABadCab5;
                
                hashCode = hashCode * 23 ^ Objects.hashCode(value.getIopsMax());
                hashCode = hashCode * 23 ^ Objects.hashCode(value.getIopsMin());
                hashCode = hashCode * 23 ^ Objects.hashCode(value.getPresetID());
                hashCode = hashCode * 23 ^ Objects.hashCode(value.getPriority());
                
                return hashCode;
            }
        }
        
        private static int _hashCode(RecurrenceRule value)
        {
            if (value == null)
            {
                return 0;
            }
            else
            {
                int hashCode = 0xABadCab5;
                
                hashCode = hashCode * 23 ^ Objects.hashCode(value.getCount());
                hashCode = hashCode * 23 ^ _hashCode((WeekDays<?>)value.getDays());
                hashCode = hashCode * 23 ^ Objects.hashCode(value.getFrequency());
                hashCode = hashCode * 23 ^ _hashCode(value.getHours());
                hashCode = hashCode * 23 ^ Objects.hashCode(value.getInterval());
                hashCode = hashCode * 23 ^ _hashCode(value.getMinutes());
                hashCode = hashCode * 23 ^ _hashCode(value.getMonthDays());
                hashCode = hashCode * 23 ^ _hashCode(value.getMonths());
                hashCode = hashCode * 23 ^ _hashCode(value.getPosition());
                hashCode = hashCode * 23 ^ _hashCode(value.getSeconds());
                hashCode = hashCode * 23 ^ _hashCode(value.getWeekNo());
                hashCode = hashCode * 23 ^ Objects.hashCode(value.getUntil());
                hashCode = hashCode * 23 ^ Objects.hashCode(value.getWeekStartDay());
                hashCode = hashCode * 23 ^ _hashCode(value.getYearDays());
                
                return hashCode;
            }
        }
        
        private static int _hashCode(Size value)
        {
            if (value == null)
            {
                return 0;
            }
            else
            {
                int hashCode = 0xABadCab5;
                
                hashCode = hashCode * 23 ^ Objects.hashCode(value.getUnit());
                hashCode = hashCode * 23 ^ Objects.hashCode(value.getValue());
                
                return hashCode;
            }
        }
        
        private static int _hashCode(SnapshotPolicy value)
        {
            if (value == null)
            {
                return 0;
            }
            else
            {
                int hashCode = 0xABadCab5;
                
                hashCode = hashCode * 23 ^ Objects.hashCode(value.getId());
                hashCode = hashCode * 23 ^ Objects.hashCode(value.getName());
                hashCode = hashCode * 23 ^ _hashCode(value.getRecurrenceRule());
                hashCode = hashCode * 23 ^ Objects.hashCode(value.getRetentionTime());
                hashCode = hashCode * 23 ^ Objects.hashCode(value.getType());
                
                return hashCode;
            }
        }
        
        private static int _hashCode(Tenant value)
        {
            if (value == null)
            {
                return 0;
            }
            else
            {
                int hashCode = 0xABadCab5;
                
                hashCode = hashCode * 23 ^ Objects.hashCode(value.getId());
                hashCode = hashCode * 23 ^ Objects.hashCode(value.getName());
                
                return hashCode;
            }
        }
        
        private static int _hashCode(VolumeAccessPolicy value)
        {
            if (value == null)
            {
                return 0;
            }
            else
            {
                int hashCode = 0xABadCab5;
                
                hashCode = hashCode * 23 ^ Objects.hashCode(value.isExclusiveRead());
                hashCode = hashCode * 23 ^ Objects.hashCode(value.isExclusiveWrite());
                
                return hashCode;
            }
        }
        
        private static int _hashCode(VolumeSettings value)
        {
            if (value == null)
            {
                return 0;
            }
            else
            {
                if (value.getClass() == VolumeSettingsBlock.class)
                {
                    return _hashCode((VolumeSettingsBlock)value);
                }
                else if (value.getClass() == VolumeSettingsObject.class)
                {
                    return _hashCode((VolumeSettingsObject)value);
                }
                else
                {
                    int hashCode = 0xABadCab5;
                    
                    hashCode = hashCode * 23 ^ Objects.hashCode(value.getVolumeType());
                    
                    return hashCode;
                }
            }
        }
        
        private static int _hashCode(VolumeSettingsBlock value)
        {
            if (value == null)
            {
                return 0;
            }
            else
            {
                int hashCode = 0xABadCab5;
                
                hashCode = hashCode * 23 ^ Objects.hashCode(value.getVolumeType());
                hashCode = hashCode * 23 ^ _hashCode(value.getBlockSize());
                hashCode = hashCode * 23 ^ _hashCode(value.getCapacity());
                
                return hashCode;
            }
        }
        
        private static int _hashCode(VolumeSettingsObject value)
        {
            if (value == null)
            {
                return 0;
            }
            else
            {
                int hashCode = 0xABadCab5;
                
                hashCode = hashCode * 23 ^ Objects.hashCode(value.getVolumeType());
                hashCode = hashCode * 23 ^ _hashCode(value.getMaxObjectSize());
                
                return hashCode;
            }
        }
        
        private static int _hashCode(VolumeStatus value)
        {
            if (value == null)
            {
                return 0;
            }
            else
            {
                int hashCode = 0xABadCab5;
                
                hashCode = hashCode * 23 ^ _hashCode(value.getCurrentUsage());
                hashCode = hashCode * 23 ^ Objects.hashCode(value.getLastCapacityFirebreak());
                hashCode = hashCode * 23 ^ Objects.hashCode(value.getLastPerformanceFirebreak());
                hashCode = hashCode * 23 ^ Objects.hashCode(value.getState());
                hashCode = hashCode * 23 ^ Objects.hashCode(value.getTimestamp());
                
                return hashCode;
            }
        }
        
        private static int _hashCode(WeekDays<?> value)
        {
            if (value == null)
            {
                return 0;
            }
            else
            {
                int hashCode = 0xABadCab5;
                
                hashCode = hashCode * 23 ^ Objects.hashCode(value);
                hashCode = hashCode * 23 ^ _hashCode(value.validator());
                
                return hashCode;
            }
        }
        
        private static int _hashCode(WeekDaysValidator<?> value)
        {
            if (value == null)
            {
                return 0;
            }
            else
            {
                int hashCode = 0xABadCab5;
                
                hashCode = hashCode * 23 ^ Objects.hashCode(value.getClass());
                
                return hashCode;
            }
        }
    }
    
    public SystemContent()
    {
        _objectNames = new HashMap<>();
        _tenants = new HashSet<>();
        _users = new HashSet<>();
        _volumeNames = new HashMap<>();
        _volumes = new HashMap<>();
    }
    
    public void addTenant(Tenant tenant)
    {
        if (tenant == null) throw new NullArgumentException("tenant");
        
        _tenants.add(tenant);
    }
    
    public void addUser(User user)
    {
        if (user == null) throw new NullArgumentException("user");
        
        _users.add(user);
    }
    
    public void addVolume(VolumeWrapper volume)
    {
        if (volume == null) throw new NullArgumentException("volume");
        
        _volumes.put(volume, new HashSet<>());
        _volumeNames.put(volume.getVolume().getName(), volume);
    }
    
    @Override
    public boolean equals(Object other)
    {
        if (other == null || other.getClass() != SystemContent.class)
        {
            return false;
        }
        else
        {
            SystemContent typedOther = (SystemContent)other;
            return _tenants.equals(typedOther._tenants)
                   && _users.equals(typedOther._users)
                   && _volumes.equals(typedOther._volumes);
        }
    }
    
    public Set<String> getObjectNames(VolumeWrapper volume)
    {
        if (volume == null) throw new NullArgumentException("volume");
        
        Set<ObjectManifest> volumeContent = getVolumeContent(volume);

        return volumeContent.stream()
                            .map(manifest -> manifest.getName())
                            .collect(Collectors.toSet());
    }
    
    public Set<String> getObjectNames(String volumeName)
    {
        if (volumeName == null) throw new NullArgumentException("volumeName");
        
        return getObjectNames(getVolume(volumeName));
    }
    
    public VolumeWrapper getVolume(String volumeName)
    {
        if (volumeName == null) throw new NullArgumentException("volumeName");
        
        VolumeWrapper volume = _volumeNames.get(volumeName);
        if (volume == null)
        {
            throw new IllegalArgumentException(
                    "volume " + javaString(volumeName) + " does not exist.");
        }

        return volume;
    }
    
    public Set<ObjectManifest> getVolumeContent(VolumeWrapper volume)
    {
        if (volume == null) throw new NullArgumentException("volume");
        
        Set<ObjectManifest> volumeContent = _volumes.get(volume);
        if (volumeContent == null)
        {
            throw new IllegalArgumentException(
                    "volume " + javaString(volume.getVolume().getName()) + " does not exist.");
        }
        
        return volumeContent;
    }
    
    public Consumer<String> getVolumeObjectNameAdder(VolumeWrapper volume)
    {
        if (volume == null) throw new NullArgumentException("volume");
        
        Consumer<ObjectManifest> volumeObjectAdder = getVolumeObjectAdder(volume);
        
        return objectName -> volumeObjectAdder.accept(new ObjectManifest.ConcreteBuilder()
                                                                        .setName(objectName)
                                                                        .build());
    }
    
    public Consumer<String> getVolumeObjectNameAdder(String volumeName)
    {
        if (volumeName == null) throw new NullArgumentException("volumeName");
        
        return getVolumeObjectNameAdder(getVolume(volumeName));
    }
    
    public Consumer<ObjectManifest> getVolumeObjectAdder(VolumeWrapper volume)
    {
        if (volume == null) throw new NullArgumentException("volume");
        
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
    
    public Consumer<ObjectManifest> getVolumeObjectAdder(String volumeName)
    {
        if (volumeName == null) throw new NullArgumentException("volumeName");
        
        VolumeWrapper volume = getVolume(volumeName);
        return getVolumeObjectAdder(volume);
    }
    
    public Set<VolumeWrapper> getVolumes()
    {
        return Collections.unmodifiableSet(_volumes.keySet());
    }
    
    public void setTenants(Collection<Tenant> value)
    {
        if (value == null) throw new NullArgumentException("value");
        
        _tenants.clear();
        for (Tenant tenant : value)
        {
            addTenant(tenant);
        }
    }
    
    @Override
    public int hashCode()
    {
        int retval = 0xABadCab5;
        
        retval = retval * 23 ^ _tenants.hashCode();
        retval = retval * 23 ^ _users.hashCode();
        retval = retval * 23 ^ _volumes.hashCode();
        
        return retval;
    }
    
    public void setUsers(Collection<User> value)
    {
        if (value == null) throw new NullArgumentException("value");
        
        _users.clear();
        for (User user : value)
        {
            addUser(user);
        }
    }
    
    public void setVolumes(Collection<VolumeWrapper> value)
    {
        if (value == null) throw new NullArgumentException("value");
        
        _volumes.clear();
        for (VolumeWrapper volume : value)
        {
            addVolume(volume);
        }
    }
    
    private final Map<String, ObjectManifest> _objectNames;
    
    private final Set<Tenant> _tenants;

    private final Set<User> _users;

    private final Map<String, VolumeWrapper> _volumeNames;

    private final Map<VolumeWrapper, Set<ObjectManifest>> _volumes;
}

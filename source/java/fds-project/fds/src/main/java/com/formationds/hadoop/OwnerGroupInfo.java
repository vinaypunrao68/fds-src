package com.formationds.hadoop;

import com.formationds.protocol.BlobDescriptor;
import org.apache.hadoop.security.UserGroupInformation;

import java.io.IOException;
import java.util.Map;

public class OwnerGroupInfo {
    private final String owner;
    private final String group;

    public OwnerGroupInfo(String owner, String group) {
        this.owner = owner;
        this.group = group;
    }

    public OwnerGroupInfo(BlobDescriptor descriptor) throws IOException {
        Map<String, String> metadata = descriptor.getMetadata();
        if (metadata.containsKey(FdsFileSystem.CREATED_BY_USER) && metadata.containsKey(FdsFileSystem.CREATED_BY_GROUP)) {
            owner = metadata.get(FdsFileSystem.CREATED_BY_USER);
            group = metadata.get(FdsFileSystem.CREATED_BY_GROUP);
        } else {
            UserGroupInformation ugi = UserGroupInformation.getCurrentUser();
            this.owner = ugi.getShortUserName();
            this.group = ugi.getShortUserName();
        }
    }

    public static OwnerGroupInfo current() throws IOException {
        UserGroupInformation ugi = UserGroupInformation.getCurrentUser();
        return new OwnerGroupInfo(ugi.getShortUserName(), ugi.getShortUserName());
    }

    public String getOwner() {
        return owner;
    }

    public String getGroup() {
        return group;
    }
}

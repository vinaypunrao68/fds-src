package com.formationds.xdi.security;

import java.util.HashMap;
import java.util.Map;

public class XdiAcl {
    public static final String X_AMZ_ACL = "x-amz-acl";
    public static final String PUBLIC_READ = "public-read";
    public static final String PUBLIC_READ_WRITE = "public-read-write";
    public static final String PRIVATE = "private";

    private static Map<String, Value> byCommonName;

    static {
        byCommonName = new HashMap<>();
        byCommonName.put(PUBLIC_READ, Value.publicRead);
        byCommonName.put(PUBLIC_READ_WRITE, Value.publicReadWrite);
        byCommonName.put(PRIVATE, Value.privateAcl);
    }

    public static enum Value {
        publicRead(PUBLIC_READ) {
            @Override
            public boolean allow(Intent intent) {
                return intent.equals(Intent.read);
            }
        },

        publicReadWrite(PUBLIC_READ_WRITE) {
            @Override
            public boolean allow(Intent intent) {
                return intent.equals(Intent.read) || intent.equals(Intent.readWrite);
            }
        },

        privateAcl(PRIVATE) {
            @Override
            public boolean allow(Intent intent) {
                return false;
            }
        };

        private String commonName;

        Value(String commonName) {
            this.commonName = commonName;
        }

        public abstract boolean allow(Intent intent);

        public String getCommonName() {
            return commonName;
        }
    }

    public static XdiAcl.Value parse(String commonName) {
        if (commonName == null) {
            commonName = "";
        }
        return byCommonName.getOrDefault(commonName, Value.privateAcl);
    }

    public static XdiAcl.Value parse(Map<String, String> blobMetadata) {
        try {
            String aclHeader = blobMetadata.getOrDefault(X_AMZ_ACL, PRIVATE);
            return byCommonName.getOrDefault(aclHeader, Value.privateAcl);
        } catch (Throwable throwable) {
            return Value.privateAcl;
        }
    }
}

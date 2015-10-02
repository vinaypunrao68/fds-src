package com.formationds.xdi.s3;

public abstract class S3Namespace {
    public abstract String getPrefix();
    public String localName(String blobName) {
        if(!blobName.startsWith(getPrefix()))
            return null;

        return blobName.substring(getPrefix().length());
    }
    public String blobName(String localName) {
        return getPrefix() + localName;
    }
    public boolean isInNamespace(String blobName) {
        return localName(blobName) != null;
    }

    public static S3Namespace user() {
        return new S3Namespace() {
            @Override
            public String getPrefix() {
                return "user:";
            }
        };
    }

    public static S3Namespace fds() {
        return new S3Namespace() {
            @Override
            public String getPrefix() {
                return "fds:";
            }
        };
    }
}

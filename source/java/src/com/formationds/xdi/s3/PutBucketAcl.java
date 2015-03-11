package com.formationds.xdi.s3;

import com.formationds.spike.later.HttpContext;
import com.formationds.spike.later.SyncRequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.formationds.xdi.Xdi;

public class PutBucketAcl implements SyncRequestHandler {
    private Xdi xdi;

    public PutBucketAcl(Xdi xdi) {
        this.xdi = xdi;
    }

    @Override
    public Resource handle(HttpContext ctx) throws Exception {
        String bucketName = ctx.getRouteParameter("bucket");

        String aclName = ctx.getRequestHeader(PutObjectAcl.X_AMZ_ACL).toLowerCase();
        if (PutObjectAcl.PUBLIC_READ.equals(aclName) ||
                PutObjectAcl.PUBLIC_READ_WRITE.equals(aclName) ||
                PutObjectAcl.PRIVATE.equals(aclName)) {
            xdi.getAuthorizer().updateBucketAcl(bucketName, aclName);
        }

        return new TextResource("");
    }
}

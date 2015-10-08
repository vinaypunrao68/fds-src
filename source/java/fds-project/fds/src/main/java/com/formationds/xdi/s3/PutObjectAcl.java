package com.formationds.xdi.s3;

import com.formationds.security.AuthenticationToken;
import com.formationds.spike.later.HttpContext;
import com.formationds.spike.later.SyncRequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.formationds.xdi.Xdi;
import com.formationds.xdi.security.XdiAcl;

public class PutObjectAcl implements SyncRequestHandler {
    private Xdi xdi;
    private AuthenticationToken token;

    public PutObjectAcl(Xdi xdi, AuthenticationToken token) {
        this.xdi = xdi;
        this.token = token;
    }

    @Override
    public Resource handle(HttpContext ctx) throws Exception {
        String bucketName = ctx.getRouteParameter("bucket");
        String objectName = ctx.getRouteParameter("object");

        String aclName = ctx.getRequestHeader(XdiAcl.X_AMZ_ACL).toLowerCase();
        XdiAcl.Value acl = XdiAcl.parse(aclName);
        xdi.getAuthorizer().updateBlobAcl(token, bucketName, S3Namespace.user().blobName(objectName), acl);
        return new TextResource("");
    }

}

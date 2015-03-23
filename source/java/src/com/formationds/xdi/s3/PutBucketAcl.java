package com.formationds.xdi.s3;

import com.formationds.security.AuthenticationToken;
import com.formationds.spike.later.HttpContext;
import com.formationds.spike.later.SyncRequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.formationds.xdi.Xdi;
import com.formationds.xdi.security.XdiAcl;

public class PutBucketAcl implements SyncRequestHandler {
    private AuthenticationToken token;
    private Xdi xdi;

    public PutBucketAcl(Xdi xdi, AuthenticationToken token) {
        this.token = token;
        this.xdi = xdi;
    }

    @Override
    public Resource handle(HttpContext ctx) throws Exception {
        String bucketName = ctx.getRouteParameter("bucket");
        String acl = ctx.getRequestHeader(XdiAcl.X_AMZ_ACL).toLowerCase();
        xdi.getAuthorizer().updateBucketAcl(token, bucketName, XdiAcl.parse(acl));
        return new TextResource("");
    }
}

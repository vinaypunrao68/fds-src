package com.formationds.xdi.s3;

import com.formationds.security.AuthenticationToken;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.formationds.xdi.Xdi;
import org.eclipse.jetty.server.Request;

import java.util.Map;

public class PutBucketAcl implements RequestHandler {
    private Xdi xdi;
    private AuthenticationToken token;

    public PutBucketAcl(Xdi xdi, AuthenticationToken token) {
        this.xdi = xdi;
        this.token = token;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String bucketName = requiredString(routeParameters, "bucket");

        String aclName = request.getHeader(PutObjectAcl.X_AMZ_ACL).toLowerCase();
        if (PutObjectAcl.PUBLIC_READ.equals(aclName) ||
                PutObjectAcl.PUBLIC_READ_WRITE.equals(aclName) ||
                PutObjectAcl.PRIVATE.equals(aclName)) {
            xdi.getAuthorizer().updateBucketAcl(bucketName, aclName);
        }

        return new TextResource("");
    }
}

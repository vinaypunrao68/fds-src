package com.formationds.util.s3.auth;

import com.amazonaws.auth.AWSCredentials;
import com.formationds.spike.later.HttpContext;
import com.formationds.util.s3.auth.stream.ChunkAuthenticationInputStream;
import com.formationds.util.s3.auth.stream.FullContentAuthenticationInputStream;
import java.io.InputStream;
import java.text.MessageFormat;
import java.util.Arrays;
import java.util.Optional;

public class AuthenticationNormalizer {

    public static final String AUTHORIZATION = "Authorization";

    public AuthenticationMode authenticationMode(HttpContext ctx) {
        String authHeader = ctx.getRequestHeader(AUTHORIZATION);
        if(authHeader == null) {
            return AuthenticationMode.None;
        }

        String[] authParts = authHeader.trim().split("\\s+", 2);
        switch(authParts[0].toUpperCase()) {
            case "AWS":
                return AuthenticationMode.v2;
            case "AWS4-HMAC-SHA256": {
                String sha = ctx.getRequestHeader("x-amz-content-sha256");
                if(sha.equalsIgnoreCase("STREAMING-AWS4-HMAC-SHA256-PAYLOAD"))
                    return AuthenticationMode.v4Chunked;
                else
                    return AuthenticationMode.v4FullRequest;
            }
        }

        return AuthenticationMode.Unknown;
    }

    public Optional<String> getPrincipalName(HttpContext ctx) {
        String authHeader =  ctx.getRequestHeader(AUTHORIZATION);
        if(authHeader == null)
            return Optional.empty();

        switch(authenticationMode(ctx)) {
            case v2:
                String pattern = "AWS {0}:{1}";
                try {
                    Object[] parsed = new MessageFormat(pattern).parse(authHeader);
                    return Optional.of((String) parsed[0]);
                } catch (Exception e) {
                    throw new SecurityException("invalid credentials");
                }
            case v4Chunked:
            case v4FullRequest:
                V4AuthHeader header = new V4AuthHeader(authHeader);
                return Optional.of(header.getUserId());
        }

        return Optional.empty();
    }

    public HttpContext authenticatingContext(AWSCredentials credentials, HttpContext ctx) throws SecurityException {
        String authHeader =  ctx.getRequestHeader(AUTHORIZATION);
        switch(authenticationMode(ctx)) {
            case v2: {
                String expected = S3SignatureGeneratorV2.hash(ctx, credentials);
                 if(!expected.equals(authHeader))
                     throw new SecurityException("Signature mismatch (v2)");
                return ctx;
            } case v4FullRequest: {
                SignatureRequestData srd = new SignatureRequestData(ctx);
                V4AuthHeader v4header = new V4AuthHeader(authHeader);

                InputStream validatingInputStream =
                        new FullContentAuthenticationInputStream(ctx.getInputStream(), credentials, srd, v4header.getScope(), v4header.getSignatureBytes());
                return ctx.withInputWrapper(validatingInputStream);
            } case v4Chunked:
                SignatureRequestData srd = new SignatureRequestData(ctx);
                V4AuthHeader header = new V4AuthHeader(authHeader);
                ChunkSignatureSequence css = new ChunkSignatureSequence(credentials.getAWSSecretKey(), srd, header.getScope());
                if(!Arrays.equals(css.getSeedSignature(), header.getSignatureBytes()))
                    throw new SecurityException("chunked mode signature mismatch");

                InputStream validatingInputStream =
                        new ChunkAuthenticationInputStream(ctx.getInputStream(), css);
                return ctx.withInputWrapper(validatingInputStream);
            case Unknown:
                throw new SecurityException("unknown authentication scheme");
            case None:
                throw new SecurityException("no authorization header present");
        }

        throw new SecurityException("unsupported authentication scheme");
    }

    public enum AuthenticationMode {
        None,
        v2,
        v4FullRequest,
        v4Chunked,
        Unknown
    }
}

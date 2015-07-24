package com.formationds.util.s3.auth;

import org.apache.commons.codec.DecoderException;
import org.apache.commons.codec.binary.Hex;

public class V4AuthHeader {
    public static final String AWS4_HMAC_SHA256 = "AWS4-HMAC-SHA256";
    private final String credential;
    private final String[] signedHeaders;
    private final String signature;

    private final String userId;
    private final ScopeInfo scope;

    public V4AuthHeader(String authHeaderValue) {
        String[] topLevelComponents = authHeaderValue.split("\\s+", 2);
        if(topLevelComponents.length != 2)
            throw new IllegalArgumentException("Authentication header is invalid, expecting two whitespace delimited parts " + formatHeaderValueForException(authHeaderValue));

        String authenticationScheme = topLevelComponents[0];
        if(!authenticationScheme.equals(AWS4_HMAC_SHA256))
            throw new IllegalArgumentException("Authentication header is not an AWS auth header " + formatHeaderValueForException(authHeaderValue));

        String signatureContent = topLevelComponents[1];
        String[] signatureComponents = signatureContent.split(",");

        if(signatureComponents.length != 3)
            throw new IllegalArgumentException("Authentication header is invalid, expecting 3 comma delimited sections in the latter part of the header value " + formatHeaderValueForException(authHeaderValue));

        String[] credentialParts = signatureComponents[0].split("=", 2);
        if(credentialParts.length != 2 || !credentialParts[0].toLowerCase().equalsIgnoreCase("Credential"))
            throw new IllegalArgumentException("Authentication header is invalid, expecting first section of signature data to contain credential information (Credential=VALUE) " + formatHeaderValueForException(authHeaderValue));

        credential = credentialParts[1];

        String[] signedHeaderParts = signatureComponents[1].split("=", 2);
        if(signedHeaderParts.length != 2 || !signedHeaderParts[0].trim().equalsIgnoreCase("SignedHeaders"))
            throw new IllegalArgumentException("Authentication header is invalid, expecting second section of signature data to contain signed headers list (SignedHeaders=HEADERS) " + formatHeaderValueForException(authHeaderValue));

        signedHeaders = signedHeaderParts[1].trim().split(";");

        String[] signatureParts = signatureComponents[2].split("=", 2);
        if(signatureParts.length != 2 || !signatureParts[0].trim().equalsIgnoreCase("Signature"))
            throw new IllegalArgumentException("Authentication header is invalid, expecting third section of signature data to contain signature (Signature=SIGNATURE) " + formatHeaderValueForException(authHeaderValue));

        signature = signatureParts[1].trim();

        String[] credentialElements =  credential.split("/");
        if(credentialElements.length != 5)
            throw new IllegalArgumentException("Authentication header is invalid, credential has improper format (expecting 5 slash delimited elements)");
        if(!credentialElements[4].equals("aws4_request"))
            throw new IllegalArgumentException("Authentication header is invalid, expecting final element of credential to be aws4_request");

        userId = credentialElements[0];
        String scope = String.join("/", credentialElements[1], credentialElements[2], credentialElements[3], credentialElements[4]);
        this.scope = new ScopeInfo(credentialElements[1], credentialElements[2], credentialElements[3]);

    }

    private String formatHeaderValueForException(String value) {
        return "[header value:" + value + "]";
    }


    public static boolean isAwsV4(String authHeaderValue) {
        String[] topLevelComponents = authHeaderValue.split("\\s+", 2);
        return topLevelComponents.length == 2 && topLevelComponents[0].toUpperCase().equals(AWS4_HMAC_SHA256);
    }

    public String getCredential() {
        return credential;
    }

    public String[] getSignedHeaders() {
        return signedHeaders;
    }

    public String getSignature() {
        return signature;
    }

    public byte[] getSignatureBytes() {
        try {
            return Hex.decodeHex(signature.toCharArray());
        } catch (DecoderException e) {
            throw new SecurityException("malformed key");
        }
    }

    public String getUserId() {
        return userId;
    }

    public ScopeInfo getScope() {
        return scope;
    }
}

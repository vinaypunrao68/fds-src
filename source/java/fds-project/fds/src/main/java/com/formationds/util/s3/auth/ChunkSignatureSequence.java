package com.formationds.util.s3.auth;

public class ChunkSignatureSequence {
    private final String secretKey;
    private final SignatureRequestData requestData;
    private final ScopeInfo scopeInfo;
    private S3SignatureGeneratorV4 signatureGeneratorV4;

    public ChunkSignatureSequence(String secretKey, SignatureRequestData requestData, ScopeInfo scopeInfo) {
        signatureGeneratorV4 = new S3SignatureGeneratorV4();
        this.secretKey = secretKey;
        this.requestData = requestData;
        this.scopeInfo = scopeInfo;
    }

    public byte[] getSeedSignature() {
        return signatureGeneratorV4.seedSignature(secretKey, requestData, scopeInfo);
    }

    public byte[] computeNextSignature(byte[] priorSignature, byte[] chunkHash) {
        return signatureGeneratorV4.chunkSignature(secretKey, requestData, scopeInfo, priorSignature, chunkHash);
    }
}

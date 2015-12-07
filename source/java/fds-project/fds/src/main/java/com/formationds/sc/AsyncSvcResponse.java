package com.formationds.sc;

import com.formationds.protocol.svc.types.AsyncHdr;

import java.nio.ByteBuffer;

public class AsyncSvcResponse {
    AsyncHdr header;
    ByteBuffer payload;

    public AsyncSvcResponse(AsyncHdr header, ByteBuffer payload) {
        this.header = header;
        this.payload = payload;
    }

    public AsyncHdr getHeader() {
        return header;
    }

    public ByteBuffer getPayload() {
        return payload;
    }
}

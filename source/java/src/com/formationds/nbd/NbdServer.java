package com.formationds.nbd;/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import io.netty.buffer.ByteBuf;
import io.netty.buffer.Unpooled;
import io.netty.channel.ChannelFuture;
import io.netty.channel.ChannelFutureListener;
import io.netty.channel.ChannelHandlerContext;
import io.netty.handler.codec.ByteToMessageDecoder;

import java.util.List;
import java.util.concurrent.CompletableFuture;

// nbd ref: https://github.com/yoe/nbd/blob/master/doc/proto.txt
public class NbdServer extends ByteToMessageDecoder {
    static final long MAGIC_NUMBER = 0x49484156454F5054l;
    static final long REPLY_MAGIC_NUMBER = 0x3e889045565a9l;
    static final int RESPONSE_MAGIC_NUMBER = 0x67446698;

    static final int NBD_CMD_READ = 0;
    static final int NBD_CMD_WRITE = 1;
    static final int NBD_CMD_DISC = 2;
    static final int NBD_CMD_FLUSH = 3;
    static final int NBD_CMD_TRIM = 4;

    static final int NBD_REP_ACK = 1;
    static final int NBD_REP_SERVER = 2;

    static final int NBD_OPT_EXPORT_NAME = 1;
    static final int NBD_OPT_ABORT = 2;
    static final int NBD_OPT_LIST = 3;

    static final int NBD_REP_ERR_UNSUP = 0x80000001;
    static final int NBD_REP_ERR_POLICY = 0x80000002;
    static final int NBD_REP_ERR_INVALID = 0x80000003;
    static final int NBD_REP_ERR_PLATFORM = 0x80000004;

    static final int NBD_FLAG_HAS_FLAGS = 0x1 << 0;
    static final int NBD_FLAG_READ_ONLY = 0x1 << 1;
    static final int NBD_FLAG_SEND_FLUSH = 0x1 << 2;
    static final int NBD_FLAG_SEND_FUA = 0x1 << 3;

    static final int NBD_FLAG_ROTATIONAL = 0x1 << 4;
    static final int NBD_FLAG_SEND_TRIM = 0x1 << 5;

    static final int NBD_FLAG_FIXED_NEWSTYLE = 0x1;

    static final byte[] NBD_MAGIC_PWD = new byte[]{'N', 'B', 'D', 'M', 'A', 'G', 'I', 'C'};

    private ProtocolState protocolState;
    private String exportName;
    private NbdServerOperations operations;

    enum ProtocolState {
        PreInit,
        PostInit,
        AwaitingOptions,
        PerformingOperations,
    }

    public NbdServer(NbdServerOperations operations) {
        this.operations = operations;
        protocolState = ProtocolState.PreInit;
    }

    @Override
    public void channelActive(ChannelHandlerContext ctx) throws Exception {
        super.channelActive(ctx);
        ByteBuf m = ctx.alloc().buffer(18);
        m.writeBytes(NBD_MAGIC_PWD);
        m.writeLong(MAGIC_NUMBER);
        m.writeShort(1);  // specify fixed new style protocol

        changeStateAfter(ctx.writeAndFlush(m), ProtocolState.PostInit);
    }

    @Override
    protected void decode(ChannelHandlerContext ctx, ByteBuf in, List<Object> out) throws Exception {
        if(protocolState == protocolState.PerformingOperations && in.readableBytes() >= 28) {
            if(in.getInt(4) == NBD_CMD_WRITE && in.readableBytes() < 28 + in.getInt(24))
                return;

            int magic = in.readInt();
            int cmd = in.readInt();
            long handle = in.readLong();
            long offset = in.readLong();
            int length = in.readInt();

            int errno = 0;
            ByteBuf readBuf = null;

            CompletableFuture<Void> ioTask = null;
            try {
                switch (cmd) {
                    case NBD_CMD_WRITE:
                        // FIXME: don't copy these bytes
                        ByteBuf writeBuf = Unpooled.buffer(length);
                        in.readBytes(writeBuf, length);
                        ioTask = operations.write(exportName, writeBuf, offset, length);
                        break;
                    case NBD_CMD_READ:
                        readBuf = Unpooled.buffer(length);
                        ioTask = operations.read(exportName, readBuf, offset, length);
                        break;
                    case NBD_CMD_FLUSH:
                        ioTask = operations.flush(exportName);
                        break;
                    default:
                        throw new Exception("Unknown NBD command");
                }
            } catch(Exception e) {
                ioTask = new CompletableFuture<>();
                ioTask.completeExceptionally(e);
            }

            final ByteBuf readBufFinal = readBuf;
            ioTask.thenRunAsync(() -> {
                ByteBuf buf = ctx.alloc().buffer();
                buf.writeInt(RESPONSE_MAGIC_NUMBER);  // 32 bit magic number?
                buf.writeInt(0);
                buf.writeLong(handle);
                if (readBufFinal != null)
                    buf.writeBytes(readBufFinal, 0, length);
                ctx.writeAndFlush(buf);
            });

            ioTask.exceptionally(ex -> {
                ByteBuf buf = ctx.alloc().buffer();
                buf.writeInt(RESPONSE_MAGIC_NUMBER);  // 32 bit magic number?
                buf.writeInt(5); // EIO
                buf.writeLong(handle);
                ctx.writeAndFlush(buf);
                return null;
            });
            in.discardReadBytes();
        } else if(protocolState == protocolState.AwaitingOptions && in.readableBytes() >= 16) {
            handleOptionMessage(ctx, in, out);
        } else if(in.readableBytes() >= 4 && protocolState == ProtocolState.PostInit) {
            in.readUnsignedInt();
            in.discardReadBytes();
            protocolState = ProtocolState.AwaitingOptions;
        }
    }

    private void handleOptionMessage(ChannelHandlerContext ctx, ByteBuf in, List<Object> out) {
        int len = in.getInt(12);
        if(in.readableBytes() < len + 16)
            return;
        byte[] option_bytes = new byte[len];
        long magic = in.readLong();
        int option_spec = in.readInt();
        in.readInt(); // bypass length
        in.readBytes(option_bytes);

        if(magic != MAGIC_NUMBER) {
            // protocol error
        }

        if(option_spec == NBD_OPT_EXPORT_NAME) {
            exportName = new String(option_bytes);
            if(!operations.exists(exportName)) {
                ctx.close();
                return;
            }

            ByteBuf m = ctx.alloc().buffer(134);
            m.writeLong(operations.size(exportName));
            m.writeShort(NBD_FLAG_HAS_FLAGS | NBD_FLAG_SEND_FLUSH | NBD_FLAG_SEND_FUA); // this is where flags go, do we need them?
            m.writeZero(124);
            ctx.writeAndFlush(m);
            in.discardReadBytes();
            protocolState = ProtocolState.PerformingOperations;
        } else if(option_spec == NBD_OPT_ABORT) {
            ctx.close();
        } else {
            ByteBuf m = ctx.alloc().buffer();
            m.writeLong(REPLY_MAGIC_NUMBER);
            m.writeInt(option_spec);
            m.writeInt(NBD_REP_ERR_UNSUP);
            m.writeInt(0);
            ctx.writeAndFlush(m);
            in.discardReadBytes();
        }
    }

    private void changeStateAfter(ChannelFuture future, ProtocolState state) {
        future.addListener(new ChannelFutureListener() {
            @Override
            public void operationComplete(ChannelFuture channelFuture) throws Exception {
                protocolState = state;
            }
        });
    }
}

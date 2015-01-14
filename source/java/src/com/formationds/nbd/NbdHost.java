package com.formationds.nbd;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import io.netty.bootstrap.ServerBootstrap;
import io.netty.channel.ChannelFuture;
import io.netty.channel.ChannelInitializer;
import io.netty.channel.ChannelOption;
import io.netty.channel.EventLoopGroup;
import io.netty.channel.nio.NioEventLoopGroup;
import io.netty.channel.socket.SocketChannel;
import io.netty.channel.socket.nio.NioServerSocketChannel;

public class NbdHost {
    private int port;
    private NbdServerOperations ops;

    public NbdHost(int port, NbdServerOperations ops) {
        this.port = port;
        this.ops = ops;
    }

    public void run() {
        EventLoopGroup connectionLoop = new NioEventLoopGroup();
        EventLoopGroup executionLoop = new NioEventLoopGroup();
        try {
            ServerBootstrap b = new ServerBootstrap();
            b.group(connectionLoop, executionLoop)
             .channel(NioServerSocketChannel.class)
             .childHandler(new ChannelInitializer<SocketChannel>() {
                 @Override
                 protected void initChannel(SocketChannel socketChannel) throws Exception {
                     socketChannel.pipeline().addLast(new NbdServer(ops));
                 }
             })
             .option(ChannelOption.SO_BACKLOG, 128)
             .childOption(ChannelOption.SO_KEEPALIVE, true);

            ChannelFuture f = b.bind(port).sync();
            f.channel().closeFuture().sync();
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
        finally {
            connectionLoop.shutdownGracefully();
            executionLoop.shutdownGracefully();
        }
    }

}

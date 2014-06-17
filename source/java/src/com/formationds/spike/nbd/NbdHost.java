package com.formationds.spike.nbd;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.AmService;
import com.formationds.apis.ConfigurationService;
import com.formationds.xdi.XdiClientFactory;
import io.netty.bootstrap.ServerBootstrap;
import io.netty.channel.ChannelFuture;
import io.netty.channel.ChannelInitializer;
import io.netty.channel.ChannelOption;
import io.netty.channel.EventLoopGroup;
import io.netty.channel.nio.NioEventLoopGroup;
import io.netty.channel.socket.SocketChannel;
import io.netty.channel.socket.nio.NioServerSocketChannel;

import java.util.concurrent.ForkJoinPool;
import java.util.concurrent.ThreadPoolExecutor;

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

    public static void main(String[] args) throws Exception {
        XdiClientFactory clientFactory = new XdiClientFactory();
        AmService.Iface am = clientFactory.remoteAmService("localhost", 9988);
        ConfigurationService.Iface config = clientFactory.remoteOmService("localhost", 9090);

        //NbdServerOperations ops;
        ForkJoinPool fjp = new ForkJoinPool(10);
        NbdServerOperations ops = new FdsServerOperations(am, config, fjp);
        //NbdServerOperations ops = new SparseRamOperations(1024L * 1024L * 1024L * 10);
        //SparseRamOperations srops = new SparseRamOperations(1024L * 1024L * 1024L * 10);
        //srops.setReadDelay(10);
        //srops.setWriteDelay(20);

        //ops = new WriteVerifyOperationsWrapper(ops);
        // ops = new OracleVerifyOperationsWrapper(srops, new SparseRamOperations(1024L * 1024L * 1024L * 10));////

        new NbdHost(10810, ops).run();
        //config.createVolume("fds", "hello", new VolumeSettings(4 * 1024, VolumeType.BLOCK, 1024 * 1024 * 1024));
        //TxDescriptor desc = am.startBlobTx("fds", "hello", "block_dev_0");
        //am.updateBlob("fds", "hello", "block_dev_0", desc, ByteBuffer.allocate(4096), 4096, new ObjectOffset(0), ByteBuffer.allocate(0), false);
        //am.commitBlobTx(desc);
    }
}

package com.formationds.om.plotter;

import org.joda.time.DateTime;
import org.junit.Test;

import javax.swing.*;
import java.awt.*;
import java.awt.image.BufferedImage;

public class VolumeStatsPlotterTest {

    @Test
    public void makeJunitHappy() {}

    public static void main(String[] args) {
        VolumeDatapoint[] datapoints = new VolumeDatapoint[]{
                new VolumeDatapoint(new DateTime(0), "foo", "size", 0),
                new VolumeDatapoint(new DateTime(1000), "foo", "size", 1),
                new VolumeDatapoint(new DateTime(2000), "foo", "size", 2),
                new VolumeDatapoint(new DateTime(0), "foo", "age", 2),
                new VolumeDatapoint(new DateTime(1000), "foo", "age", 1),
                new VolumeDatapoint(new DateTime(2000), "foo", "age", 0)
        };

        BufferedImage image = new VolumeStatsPlotter().plot("foo", datapoints);
        JFrame frame = new JFrame();
        frame.setDefaultCloseOperation(WindowConstants.EXIT_ON_CLOSE);
        frame.setSize(800, 1500);
        frame.getContentPane().add(new Panel() {
            @Override
            public void paint(Graphics g) {
                g.drawImage(image, 0, 0, null);
            }
        });

        frame.setVisible(true);
    }
}
package com.formationds.nativeapi;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.Arrays;

public class ServerSetup {
    private Process[] processes;

    public void setUp() {
        processes = Arrays.stream(new String[]{"./orchMgr", "./DataMgr", "./StorMgr"})
                .map(p -> runServer(p))
                .toArray(i -> new Process[i]);
    }

    private Process runServer(String path) {
        try {
            System.out.println("Running " + path);
            Process process = new ProcessBuilder(path).start();
            new Thread(() -> {
                try {
                    Thread.sleep(100);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                InputStream is = process.getInputStream();
                InputStreamReader isr = new InputStreamReader(is);
                BufferedReader br = new BufferedReader(isr);
                String line;

                try {
                    while ((line = br.readLine()) != null) {
                        //System.out.println("[" + path + "]" + line);
                    }
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }).start();
            return process;
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    public void tearDown() throws Exception {
        Arrays.stream(processes).forEach(p -> p.destroyForcibly());
    }

}

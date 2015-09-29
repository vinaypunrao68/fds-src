package com.formationds.spike.iscsi;
import org.jscsi.target.Configuration;
import org.jscsi.target.Target;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public class JScsiConfigurationBuilder {
    private String address;
    private boolean sloppyNegotiation;
    private int port;
    private List<Target> targetList;
    private int targetPortalGroupTag;

    public JScsiConfigurationBuilder() {
        this.port = 3260;
        address = "";  // defaults to localhost
        sloppyNegotiation = false;
        targetList = new ArrayList<>();
    }

    public JScsiConfigurationBuilder withPort(int port) {
        this.port = port;
        return this;
    }

    public JScsiConfigurationBuilder withSloppyNegotiation(boolean isSloppy) {
        sloppyNegotiation = isSloppy;
        return this;
    }

    public JScsiConfigurationBuilder withAddress(String address) {
        this.address = address;
        return this;
    }

    public JScsiConfigurationBuilder withTarget(Target tgt) {
        targetList.add(tgt);
        return this;
    }

    public Configuration build() {
        try {
            return new ProgrammaticConfiguration(this);
        } catch (IOException ex) {
            throw new AssertionError("Unexpected failure while initializing jscsi config", ex);
        }
    }

    private static class ProgrammaticConfiguration extends Configuration {
        public ProgrammaticConfiguration(JScsiConfigurationBuilder builder) throws IOException {
            super(builder.address);
            targetAddress = builder.address;
            allowSloppyNegotiation = builder.sloppyNegotiation;
            port = builder.port;
            targets.addAll(builder.targetList);
        }
    }

}

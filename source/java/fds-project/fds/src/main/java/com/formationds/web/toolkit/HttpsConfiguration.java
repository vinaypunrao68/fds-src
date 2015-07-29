package com.formationds.web.toolkit;

import com.formationds.util.Configuration;
import com.formationds.commons.libconfig.ParsedConfig;

import java.io.File;

public class HttpsConfiguration extends HttpConfiguration {
    private File keyStore;
    private String keystorePassword;
    private String keyManagerPassword;

    public HttpsConfiguration(int port, String host, File keyStore, String keystorePassword, String keyManagerPassword) {
        super(port, host);
        this.keyStore = keyStore;
        this.keystorePassword = keystorePassword;
        this.keyManagerPassword = keyManagerPassword;
    }

    public HttpsConfiguration(int port, Configuration configuration) {
        super(port);
        ParsedConfig platformConfig = configuration.getPlatformConfig();
        this.keyStore = platformConfig.getPath(Configuration.KEYSTORE_PATH, configuration.getFdsRoot());
        this.keystorePassword = platformConfig.lookup(Configuration.KEYSTORE_PASSWORD).stringValue();
        this.keyManagerPassword = platformConfig.lookup(Configuration.KEYMANAGER_PASSWORD).stringValue();
    }

    public File getKeyStore() {
        return keyStore;
    }

    public String getKeystorePassword() {
        return keystorePassword;
    }

    public String getKeyManagerPassword() {
        return keyManagerPassword;
    }
}

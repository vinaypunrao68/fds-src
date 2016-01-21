package com.formationds.nfs;

import com.formationds.apis.VolumeDescriptor;
import com.formationds.protocol.NfsOption;
import com.google.common.base.Joiner;
import org.apache.commons.lang.StringUtils;

import java.util.HashSet;
import java.util.Set;
import java.util.StringTokenizer;

/**
 * Builds an /etc/exports-style entry from an FDS VolumeDescriptor
 */
public class ExportConfigurationBuilder {
    public String buildConfigurationEntry(VolumeDescriptor vd) {
        String options = rewriteNfsOptions(vd.getPolicy().getNfsOptions());
        Set<String> clientFilters = parseClientFilters(vd.getPolicy().getNfsOptions());
        StringBuilder sb = new StringBuilder();
        sb.append("/" + vd.getName());

        for (String clientFilter : clientFilters) {
            sb.append(" ");
            sb.append(clientFilter);
            sb.append("(");
            sb.append(options);
            sb.append(")");
        }

        return sb.toString();
    }

    private Set<String> parseClientFilters(NfsOption nfsOptions) {
        Set<String> clientFilters = new HashSet<>();
        StringTokenizer tokenizer = new StringTokenizer(nfsOptions.getClient(), ",", false);
        while (tokenizer.hasMoreTokens()) {
            clientFilters.add(tokenizer.nextToken());
        }
        return clientFilters;
    }

    private String rewriteNfsOptions(NfsOption nfsOptions) {
        if (StringUtils.isEmpty(nfsOptions.getOptions())) {
            return "rw,noacl,no_root_squash";
        }
        String optionsClause = nfsOptions.getOptions();
        // Remove the yet-unsupported async option
        StringTokenizer tokenizer = new StringTokenizer(optionsClause, ",", false);
        Set<String> options = new HashSet<>();
        options.add("rw");
        while (tokenizer.hasMoreTokens()) {
            String token = tokenizer.nextToken();
            if (!token.contains("sync")) {
                options.add(token);
            }
        }
        return Joiner.on(",").join(options);
    }
}

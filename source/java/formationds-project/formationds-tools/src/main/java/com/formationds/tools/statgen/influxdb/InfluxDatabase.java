/**
 * Copyright (c) 2014 Formation Data Systems.  All rights reserved.
 */

package com.formationds.tools.statgen.influxdb;

import com.google.common.collect.Lists;
import org.influxdb.dto.DatabaseConfiguration;
import org.influxdb.dto.ShardSpace;
import org.influxdb.dto.User;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class InfluxDatabase
{
    /**
     *
     */
    public static class Builder
    {
        private final String name;
        private final List<ShardSpace> spaces            = Lists.newArrayList();
        private final List<String>     continuousQueries = Lists.newArrayList();
        private final List<User>       users             = new ArrayList<>();

        public Builder( String name ) {
            this.name = name;
        }

        /**
         * http://influxdb.com/docs/v0.8/advanced_topics/schema_design.html
         *
         * @param name the name of the shard space
         * @param retentionPolicy the retention policy for the shard space
         * @param shardDuration the shard duration
         * @param regex the regex for the Series mapped to this shard space
         * @param replicationFactor the replication factor for the shard space, if clustered
         * @param split the split factor for this shard space
         *
         * @return this
         */
        public Builder addShardSpace( final String name,
                                      final String retentionPolicy,
                                      final String shardDuration,
                                      final String regex,
                                      final int replicationFactor,
                                      final int split)
        {
            ShardSpace s = new ShardSpace( name, retentionPolicy, shardDuration, regex, replicationFactor, split );
            return addShardSpace( s );
        }

        /**
         * @param s
         * @return this
         */
        public Builder addShardSpace( ShardSpace s )
        {
            return addShardSpaces( s );
        }

        /**
         * @param s
         * @param others
         * @return this
         */
        public Builder addShardSpaces( ShardSpace s, ShardSpace... others )
        {
            spaces.add( s );
            if ( others != null )
            {
                spaces.addAll( Arrays.asList( others ) );
            }
            return this;
        }

        /**
         * @see <a href="http://influxdb.com/docs/v0.8/api/query_language.html>Query Language</a>
         * @see <a href="http://influxdb.com/docs/v0.8/api/continuous_queries.html">Continuous Queries</a>
         *
         * @param cq the query
         *
         * @return this
         */
        public Builder addContinuousQuery( String cq )
        {
            return addContinuousQueries( cq );
        }

        /**
         *
         * @param cq the query
         * @param others additional queries
         * @return this
         */
        public Builder addContinuousQueries( String cq, String... others )
        {
            continuousQueries.add( cq );
            if ( others != null )
            {
                continuousQueries.addAll( Arrays.asList( others ) );
            }
            return this;
        }

        /**
         *
         * @param user the user to add
         * @param pwd the user's password
         * @param admin true if the user is an admin user for this database
         * @param readPerm influx regular expression for read permissions
         * @param writePerm influx regular expression for write permissions
         *
         * @return this
         */
        public Builder addUser( String user, char[] pwd, boolean admin, String readPerm, String writePerm )
        {
            User u = new User( user );
            u.setAdmin( admin );
            u.setPassword( String.valueOf( pwd ) );
            u.setReadFrom( readPerm );
            u.setWriteTo( writePerm );

            users.add( u );
            return this;
        }

        public InfluxDatabase build()
        {
            return new InfluxDatabase( name, spaces, continuousQueries, users );
        }
    }

    private final String name;
    private final List<ShardSpace> spaces            = Lists.newArrayList();
    private final List<String>     continuousQueries = Lists.newArrayList();

    // TODO: user credentials need to be encrypted and I don't believe they are in this DTO from influxdb-java lib
    private final List<User> users = new ArrayList<>();

    /**
     *
     * @param name the database name
     * @param spaces the list of shard spaces
     * @param continuousQueries the list of continuous queries
     * @param users users defined in the database.
     */
    InfluxDatabase( String name, List<ShardSpace> spaces, List<String> continuousQueries, List<User> users ) {
        this.name = name;
        this.spaces.addAll( spaces );
        this.continuousQueries.addAll( continuousQueries );
        this.users.addAll( users );
    }

    public String getName() {
        return name;
    }

    public List<ShardSpace> getSpaces() {
        return spaces;
    }

    public List<String> getContinuousQueries() {
        return continuousQueries;
    }

    public List<User> getUsers()
    {
        return users;
    }

    public DatabaseConfiguration getDatabaseConfiguration()
    {
        DatabaseConfiguration db = new DatabaseConfiguration( name );
        for (ShardSpace space : spaces)
        {
            db.addSpace( space );
        }

        for (String cq : continuousQueries)
        {
            db.addContinuousQueries( cq );
        }
        return db;
    }

    InfluxDatabase addUser(User user) { users.add( user ); return this; }
    InfluxDatabase addContinuousQuery(String cq) { continuousQueries.add( cq ); return this; }
    InfluxDatabase addShardSpace(ShardSpace shardSpace) { spaces.add( shardSpace ); return this; }

}

# Formation Data Systems, INC.
This directory structure continues Maven build tree for the following projects:

* fds thrift ( artifact: fds-thrift-**version#**.jar )
* fds ( artifact: fds-**version#**.jar )
* fds deploy ( places artifacts so they can be picked up by packaging )

## Update Project and dependency versions

### Update version number ( SNAPSHOT and RELEASE )
To update the versions of the project versions
`mvn versions:set -DnewVersion=<new version>`

### Update dependency versions
To update the dependency versions, **BE VERY CAREFUL RUNNING THIS COMMAND**
`mvn versions:use-latest-versions`

## License
Copyright &copy; 2013-2015 Formation Data Systems, INC. All rights reserved.
Creating Software Packages at FDS
==================================
- To create a package , use the `fds-pkg-create` tool on package definition `.pkgdef` file.
- As of now , the pkg repository is setup on [coke](http://coke.formationds.com:8000)

Package Definition File Format
==================================
- contains information to create a package
- naming convention : `pkgname.pkgdef`
- specified in [yaml](http://en.wikipedia.org/wiki/YAML#Sample_document) format
- all fields are lower case

Sample file
-----------
```
package: fds-platformmgr
version: 0.1
maintainer: fds <engineering@formationds.com>
description: Helper scripts for pkg creation

installcontrol : fdsplatform.control

depends :
        - libc (>= 2.0)
        - libboost
files:
          dest: /fds/bin/platformd
          src : '../source/Build/linux-x86_64.debug/bin/platformd'

services:
        name: fds.platform
        control: fds.platform.service
        
cronjobs:
        interval: minutely
        command : /fds/sbin/monitorplatform.sh
```

Package Fields
--------------
- `package` : The name of the package *mandatory*.
   -  alphanumeric lower case , first letter is alpha `regex: '^[a-z0-9][a-z0-9-\.]+$'`

- `version` : The version number of the package *mandatory*.
   -  alphanumeric lower case, first letter is a digit `regex: '^[0-9][a-z0-9-\.]+$'`

- `description` : Simple description of the what the package contains *mandatory*.
   -  single line or multiline . Multiline should be specified with yaml pipe (|).

- `maintainer` : The Maintainer of the package. 
   - `default: fds <engineering@formationds.com>`
   
- `depends` : The packages that this package depends on 
   - can be just one string or a list of strings
``` 
   depends : just-one (<< 1.2)
OR
   depends :
         -  package1 (> 1.5)
         -  package2 (= 1.6)
```

- `installcontrol` : The script that provides `(pre/post)(install/remove)` functionality.
   - look in the templates directory for a [sample] (templates/sample.installcontrol)

- `files` : which files which should be copied where . Can be just one or a list of details.
    - `src` : list of files to be copied. shell globs can be specified `*.sh`.
    -  for `src` if the value starts and ends with back ticks, that will be expanded as a shell command.
    - `dest` :  the destiation location of the files.
             - `dest` by default will be the full filename
             - `dest` is absolute path
             - `dest` should be a directory if multiple files are in `src`
             - `dest` can be explicitly set as directory with `dir: True`

    - `dir` : is the destination a directory or plain file `default: False`
    - `owner` :  who owns the file `default root:root`
             - `owner` should be specified as `user:group`

    - `perms` : file permissions
             - `perms` should be specified in `chmod` format

- `services` : list of services to be installed. (just one or a list)
    - `name` : name of the service (same format as package name)
    - `control` : the control file which provides `start/stop/status/` functionality. Look into templates directory for a [sample] (templates/sample.simple.service).

- `cronjobs` : list of crons to be setup. (just one or a list)
    - `command` : shell command or script to be run
    - interval : how often should the command be run [minutely/hourly/daily/yearly ... ] or cron format.


Uploading Packages to Repo
=========================
Use the `fds-pkg-dist` tool to upload the created package to the repository

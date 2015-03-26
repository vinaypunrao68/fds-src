# Formation Data System Web Client

## Getting Started

The application uses npm and bower for dependency management. Make sure that you have both.

`$ npm install` to install all node and bower dependencies


To run the app:

`$ grunt serve` and open the browser to [localhost:9000](http://localhost:9000)

In order to proxy to a real node you will need to open Gruntfile.js and edit the variable proxySetting and change the host accordingly.

## Build Targets

####grunt (or grunt dev)  

This will compile the code into the `app` directory in largely the same format as in the src directory.  This is for ease of debugging javascript files which all retain the same name and line numbers.  The CSS will be combined with SASS, but will not be minified.

####grunt serve

This will compile as above, but it will also serve the source code from the `app` directory under localhost:9001

####grunt release

This will build the code in the release format.  That is to say, the javascript code will be largely combined and uglified in addition to the CSS code.  It will still be compiled under `app`.

####grunt release-serve

This builds in the same manner as `grunt release` but will serve the source from the `app` directory under localhost:9001

## FDS API

This Admin App communicates with the FDS platform to create/manage volumes, as well activate/deactivate services.

### Global Domain  `#GET /api/config/globaldomain`
```JSON
{"name": "FDS"}
```

### Domains(site and local) `#GET /local_domains`

```JSON
[
    {
        "id": 0
        "site": "Fremont",
        "name": "Formation Data Systems",
    },
    {
        "id": 1
        "site": "San Francisco",
        "name": "Formation Data Systems",
    },
    {
        "id": 2
        "site": "Boulder",
        "name": "QuickLeft, Inc.",
    }
]
```


### Nodes vs. Unattached Nodes
Both share a single endpoint, if the payload is grouped by `node_uuid`, nodes will contain an array of service objects, and unattached nodes will be alone and must have attribute `node_type: "FDSP_PLATFORM"`.

### Nodes `#GET  /local_domains/:ldomain_name/services`
__This endpoint lists the services of Local Domain nodes and unattached nodes.__
Nodes do not exist in the platform. A node in this application represents a group of services. __OM, AM, SM, DM, PM__ are the services that can be activated for a single node. There can at most be one of each service per node.
The `#parse` function in the `collections/nodes.coffee` will return nodes and reject unattached nodes. [view src](https://github.com/FDS-Dev/baboon/blob/master/app/scripts/collections/nodes.coffee#L22)

```JSON
[
    {
        "node_id": 0,
        "node_state": "FDS_Node_Up",
        "service_type": "FDSP_STOR_MGR",
        "node_name": "my-sm-node",
        "ip_hi_addr": 0,
        "ip_lo_addr": 0,
        "control_port": 7012,
        "data_port": 7012,
        "migration_port": 7013,
        "node_uuid": "7519163938798932937",
        "service_uuid": "7519163938798932938"
        "node_root": "node_root",
        "metasync_port": 0
    }
]
```

### Unattached Nodes `#GET  /api/config/services`
__This endpoint lists both nodes and unattached nodes.__
The `#parse` function in the `collections/unattached_nodes.coffee` will return unattached nodes and reject nodes. [view src](https://github.com/FDS-Dev/baboon/blob/master/app/scripts/collections/unattached_nodes.coffee#L14)
Unattached Nodes are single available services that are ready to be activated.

### Volumes `/api/config/volumes`

This endpoint is responsible for all volume actions.
`#GET /api/config/volumes` returns an array of volume objects.
`#POST /api/config/volumes` creates a new volume.
`#PUT /api/config/volumes/:volume_id` updates an existing volume.

```JSON
{
  "name": "FDScats",
  "priority": 1,
  "sla": 10,
  "limit": 300,
  "data_connector": {
    "type": "block"
    "name": "cinder"
    "attributes": {
      "size": "100"
      "unit": "TB"
    }
  }
}

{
  "name": "FDScats",
  "priority": 1,
  "sla": 10,
  "limit": 300,
  "data_connector": {
    "type": "object"
    "name": "S3"
  }
}


```


## Testing

To test the you can run `grunt unittest` to run the complete Karma unit test suite.

To run the e2e protractor tests you will need to run one of the two options to serve the site, and in a second terminal you can run `grunt e2e` and it will launch a Chrome browser to run the end to end tests against.

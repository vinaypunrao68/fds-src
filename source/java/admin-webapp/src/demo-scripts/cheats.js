var setFirebreak = function( volumeName, state ){
    
    var volumes = JSON.parse( window.localStorage.getItem( 'volumes' ) );
    var volume;
    
    for ( var i = 0; i < volumes.length; i++ ){
        if ( volumes[i].name === volumeName ){
            volume = volumes[i];
            break;
        }
    }
    
    if ( !angular.isDefined( volumes ) ){
        console.log( 'No volume found with that name' );
        return;
    }
    
    var time = ((new Date()).getTime() / 1000).toFixed( 0 );
    
    if ( !angular.isDefined( state ) ){
        state = 'red';
    }
    
    switch( state ){
        case 'orange':
            time -= 3600;
            break;
        case 'yellow':
            time -= 3*3600;
            break;
        case 'lime':
            time -= 6*3600;
            break;
        case 'lightgreen':
            time -= 12*3600;
            break;
        case 'green':
            time = 0;
            break;
        default:
            break;
    }
    
    var fb = { time: time };
    
    window.localStorage.setItem( volume.id + '_fb', JSON.stringify( fb ) );
    
};

var showVolume = function( name ){
    
    var volumes = JSON.parse( window.localStorage.getItem( 'volumes' ) );
    
    for ( var i = 0; i < volumes.length; i++ ){
        
        if ( name === volumes[i].name ){
            console.log( JSON.stringify( volumes[i] ) );
            break;
        }
    }
};

var listVolumes = function( full ){
    
    if ( !angular.isDefined( full ) ){
        full = false;
    }
    
    var volumes = JSON.parse( window.localStorage.getItem( 'volumes' ) );
    
    for ( var i = 0; i < volumes.length; i++ ){
        
        var str = volumes[i].uid + ': ' + volumes[i].name;
        
        if ( full === true ){
            str = JSON.stringify( volumes[i] );
        }
        
        console.log( str );
    }
};

var addNode = function( name ){
    
    var newNode = {
        "name": name,
        "uid": (new Date()).getTime(),
        "address": {
            "ipv4Address": "10.3.50." + (Math.random() * 255),
            "ipv6Address": "nothing"
        },
        "state": "DISCOVERED",
        "serviceMap": {
            "PM": [
                {
                    "uid": (new Date()).getTime() + 1,
                    "name": "PM",
                    "port": 7031,
                    "status": {
                        "state": "NOT_RUNNING",
                        "errorCode": 1000,
                        "description": "Not added."
                    },
                    "type": "PM"
                }
            ]
        }
    };
    
    var detachedNodes = JSON.parse( window.localStorage.getItem( 'detachedNodes' ) );
    
    if ( detachedNodes === null ){
        detachedNodes = [];
    }
    
    detachedNodes.push( newNode );
    
    window.localStorage.setItem( 'detachedNodes', JSON.stringify( detachedNodes ) );
};

var setServiceState = function( nodeName, services, state ){
    
    var nodes = JSON.parse( window.localStorage.getItem( 'nodes' ) );
    
    for ( var i = 0; nodes !== null && i < nodes.length; i++ ){
        
        if ( nodes[i].name !== nodeName ){
            continue;
        }
        
        if ( angular.isString( services ) ){
            services = services.split( ',' );
        }
        
        for ( var j = 0; j < services.length; j++ ){
            
            var service = nodes[i].services[services[j].toUpperCase()];

            if ( !angular.isDefined( service ) || service === null ){
                continue;
            }

            service[0].status.state = state;
        }
        break;
    }
    
    window.localStorage.setItem( 'nodes', JSON.stringify( nodes ) );
};

var listNodes = function( full ){
    
    if ( !angular.isDefined( full ) ){
        full = false;
    }
    
    var nodes = JSON.parse( window.localStorage.getItem( 'nodes' ) );
    
    for ( var i = 0; i < nodes.length; i++ ){
        var str = nodes[i].id.uuid + ': ' + nodes[i].id.name;
        
        if ( full === true ){
            str = nodes[i];
        }
        
        console.log( str );
    }
};

var listDetachedNodes = function( full ){
    
    if ( !angular.isDefined( full ) ){
        full = false;
    }
    
    var nodes = JSON.parse( window.localStorage.getItem( 'detachedNodes' ) );

    for ( var i = 0; i < nodes.length; i++ ){
        var str = nodes[i].id.uuid + ': ' + nodes[i].id.name;
        
        if ( full === true ){
            str = nodes[i];
        }
        
        console.log( str );
    }
};

var clean = function(){
    window.localStorage.clear();
};
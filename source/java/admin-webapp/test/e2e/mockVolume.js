mockVolume = function(){
    angular.module( 'volume-management', ['qos'] ).factory( '$volume_api', ['$snapshot_service', function( $snapshot_service ){

        var volService = {};
        volService.volumes = [];

        volService.delete = function( volume ){

            var temp = [];

            volService.volumes.forEach( function( v ){
                if ( v.name != volume.name ){
                    temp.push( v );
                }
            });

            volService.volumes = temp;
        };

        volService.save = function( volume, callback ){

            for ( var i = 0; i < volService.volumes.length; i++ ){
                var thisVol = volService.volumes[i];

                if ( thisVol.name === volume.name ){
                    // edit
                    volService.volumes[i] = volume;

                    if ( angular.isDefined( callback ) ){
                        callback();
                    }

                    return;
                }
            }

            volume.id = (new Date()).getTime();
            volService.volumes.push( volume );

            if ( angular.isDefined( callback ) ){
                callback( volume );
            }
        };

        volService.createSnapshot = function( volumeId, newName, callback, failure ){
            callback();
        };

        volService.deleteSnapshot = function( volumeId, snapshotId, callback, failure ){
            callback();
        };

        volService.getSnapshots = function( volumeId, callback, failure ){
            //callback( [] );
        };

        volService.getSnapshotPoliciesForVolume = function( volumeId, callback, failure ){
        //                callback( [] );
            var ps = $snapshot_service.getPolicies();
            var rtn = [];

            for ( var i = 0; i < ps.length; i++ ){
                
                if ( ps[i].name.indexOf( volumeId ) != -1 ){
                    rtn.push( ps[i] );
                }
            }
            
            if ( angular.isFunction( callback ) ){
                callback( rtn );
            }
        };

        volService.refresh = function(){};

        return volService;
    }]);
    
    
    angular.module( 'volume-management' ).factory( '$data_connector_api', function(){

        var api = {};
        api.connectors = [];

        var getConnectorTypes = function(){
    //        return $http.get( '/api/config/data_connectors' )
    //            .success( function( data ){
    //                api.connectors = data;
    //            });

            api.connectors = [{
                type: 'Block',
                api: 'Basic, Cinder',
                options: {
                    max_size: '100',
                    unit: ['GB', 'TB', 'PB']
                },
                attributes: {
                    size: '10',
                    unit: 'GB'
                }
            },
            {
                type: 'Object',
                api: 'S3, Swift'
            }];
        }();

        api.editConnector = function( connector ){

            api.connectors.forEach( function( realConnector ){
                if ( connector.type === realConnector.type ){
                    realConnector = connector;
                }
            });
        };

        return api;
    });
};

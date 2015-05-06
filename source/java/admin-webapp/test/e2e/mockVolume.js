mockVolume = function(){
    angular.module( 'volume-management', ['qos'] ).factory( '$volume_api', ['$snapshot_service', function( $snapshot_service ){

        var volService = {};
        volService.volumes = [];

        var saveVolumes = function(){
            
            if ( angular.isDefined( window ) && angular.isDefined( window.localStorage ) ){
                window.localStorage.setItem( 'volumes', JSON.stringify( volService.volumes ) );
            }
        };
        
        var saveVolumeEvent = function( volume ){
            
            if ( !angular.isDefined( window ) || !angular.isDefined( window.localStorage ) ){
                return;
            }
            
            var event = {
                userId: 1,
                id: (new Date()).getTime(),
                type: 'USER_ACTIVITY',
                category: 'VOLUMES',
                severity: 'CONFIG',
                state: 'SOFT',
                initialTimestamp: (new Date()).getTime(),
                modifiedTimestamp: (new Date()).getTime(),
                messageKey: 'CREATE_VOLUME',
                messageArgs: [
                    "",
                    "test",
                    "0",
                    "OBJECT",
                    "2097152"
                ],
                messageFormat: 'Created volume: domain\u003d{0}; name\u003d{1}; tenantId\u003d{2}; type\u003d{3}; size\u003d{4}',
                defaultMessage: 'Created volume: domain\u003d; name\u003dtest; tenantId\u003d0; type\u003dOBJECT; size\u003d2097152'
            };
            
            var events = JSON.parse( window.localStorage.getItem( 'activities' ) );
            
            if ( !angular.isDefined( events ) || events === null ){
                events = [];
            }
            
            if ( events.length > 30 ){
                events.splice( 0, 1 );
            }
            
            events.push( event );
            
            window.localStorage.setItem( 'activities', JSON.stringify( events ) );
        };
        
        volService.delete = function( volume, callback ){

            var temp = [];

            for ( var i = 0; i < volService.volumes.length; i++ ){
                if( volService.volumes[i].name !== volume.name ){
                    temp.push( volService.volumes[i] );
                }
            }

            volService.volumes = temp;
            
            saveVolumes();
            
            if ( angular.isFunction( callback ) ){
                callback();
            }
        };

        volService.save = function( volume, callback, failure ){
            
            for ( var i = 0; i < volService.volumes.length; i++ ){
                var thisVol = volService.volumes[i];

                if ( thisVol.name === volume.name ){
                    // edit
                    volService.volumes[i] = volume;

                    saveVolumes();
                    
                    if ( angular.isDefined( callback ) ){
                        callback( volume );
                    }

                    return;
                }
            }

            volume.id = (new Date()).getTime();
            volume.current_usage = {
                size: 0,
                unit: 'B'
            };
            
            volume.rate = 10000;
            volume.snapshots = [];
            volService.volumes.push( volume );
            saveVolumeEvent( volume );
            
            saveVolumes();
            
            if ( angular.isDefined( callback ) ){
                callback( volume );
            }
        };
        
        volService.clone = function( volume, callback, failure ){
            volService.save( volume, callback, failure );
        };
        
        volService.cloneSnapshot = function( volume, callback, failure ){
            volService.save( volume, callback, failure );
        };

        volService.createSnapshot = function( volumeId, newName, callback, failure ){
            
            var volume;
            
            for ( var i = 0; i < volService.volumes.length; i++ ){
                if ( volService.volumes[i].id === volumeId ){
                    volume = volService.volumes[i];
                    break;
                }
            }
            
            if ( !angular.isDefined( volume.snapshots ) ){
                volume.snapshots = [];
            }
            
            var id = (new Date()).getTime();
            volume.snapshots.push( { id: id, name: id, creation: id } );
            
            callback();
        };

        volService.deleteSnapshot = function( volumeId, snapshotId, callback, failure ){
            callback();
        };

        volService.getSnapshots = function( volumeId, callback, failure ){
            //callback( [] );
            
            for ( var i = 0; i < volService.volumes.length; i++ ){
                if ( volService.volumes[i].id === volumeId ){
                    
                    if ( angular.isFunction( callback ) ){
                        callback( volService.volumes[i].snapshots );
                    }
                    
                    return volService.volumes[i].snapshots;
                }
            }
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
            
            return {
                then: function( cb ){
                    if ( angular.isFunction( cb ) ){
                        cb( rtn );
                    }
                }
            }
        };
        
        volService.getQosPolicyPresets = function( callback, failure ){
            
            var presets = [
                {
                    priority: 10,
                    sla: 0,
                    limit: 0,
                    uuid: -1,
                    name: 'Least Important'
                },
                {
                    priority: 7,
                    sla: 0,
                    limit: 0,
                    uuid: -1,
                    name: 'Standard'
                },
                {
                    priority: 1,
                    sla: 0,
                    limit: 0,
                    uuid: -1,
                    name: 'Most Important'
                }
            ];
                
            if ( angular.isFunction( callback ) ){
                callback( presets );
            }
        };
        
        volService.getSnapshotPolicyPresets = function( callback, failure ){
            
            var presets = [{
                commitLogRetention: 86400,
                name: 'Sparse Coverage',
                policies: [{
                    recurrenceRule: {
                        FREQ: 'DAILY',
                        BYMINUTE: ['0'],
                        BYHOUR: ['0']
                    },
                    retention: 172800
                },
                {
                    recurrenceRule: {
                        FREQ: 'WEEKLY',
                        BYMINUTE: ['0'],
                        BYHOUR: ['0'],
                        BYDAY: ['MO']
                    },
                    retention: 604800
                },
                {
                    recurrenceRule: {
                        FREQ: 'MONTHLY',
                        BYMINUTE: ['0'],
                        BYHOUR: ['0'],
                        BYMONTHDAY: ['1']
                    },
                    retention: 7776000
                },
                {
                    recurrenceRule: {
                        FREQ: 'YEARLY',
                        BYMINUTE: ['0'],
                        BYHOUR: ['0'],
                        BYMONTHDAY: ['1'],
                        BYMONTH: ['1']
                    },
                    retention: 63244800
                }]
            },
            {
                commitLogRetention: 86400,
                name: 'Standard',
                policies: [{
                    recurrenceRule: {
                        FREQ: 'DAILY',
                        BYMINUTE: ['0'],
                        BYHOUR: ['0']
                    },
                    retention: 604800
                },
                {
                    recurrenceRule: {
                        FREQ: 'WEEKLY',
                        BYMINUTE: ['0'],
                        BYHOUR: ['0'],
                        BYDAY: ['MO']
                    },
                    retention: 7776000
                },
                {
                    recurrenceRule: {
                        FREQ: 'MONTHLY',
                        BYMINUTE: ['0'],
                        BYHOUR: ['0'],
                        BYMONTHDAY: ['1']
                    },
                    retention: 15552000
                },
                {
                    recurrenceRule: {
                        FREQ: 'YEARLY',
                        BYMINUTE: ['0'],
                        BYHOUR: ['0'],
                        BYMONTHDAY: ['1'],
                        BYMONTH: ['1']
                    },
                    retention: 158112000
                }]
            },
            {
                commitLogRetention: 172800,
                name: 'Dense Coverage',
                policies: [{
                    recurrenceRule: {
                        FREQ: 'DAILY',
                        BYMINUTE: ['0'],
                        BYHOUR: ['0']
                    },
                    retention: 2592000
                },
                {
                    recurrenceRule: {
                        FREQ: 'WEEKLY',
                        BYMINUTE: ['0'],
                        BYHOUR: ['0'],
                        BYDAY: ['MO']
                    },
                    retention: 18144000
                },
                {
                    recurrenceRule: {
                        FREQ: 'MONTHLY',
                        BYMINUTE: ['0'],
                        BYHOUR: ['0'],
                        BYMONTHDAY: ['1']
                    },
                    retention: 63244800
                },
                {
                    recurrenceRule: {
                        FREQ: 'YEARLY',
                        BYMINUTE: ['0'],
                        BYHOUR: ['0'],
                        BYMONTHDAY: ['1'],
                        BYMONTH: ['1']
                    },
                    retention: 474336000
                }]
            }];
            
            if ( angular.isFunction( callback ) ) {
                callback( presets );
            }
        };

        volService.refresh = function(){
            
            if ( angular.isDefined( window ) && angular.isDefined( window.localStorage ) ){
                var vols = JSON.parse( window.localStorage.getItem( 'volumes' ) );
                
                if ( !angular.isDefined( vols ) || vols === null ){
                    vols = [];
                }
                
                volService.volumes = vols;
            }
        };

        volService.refresh();
        
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

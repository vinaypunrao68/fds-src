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

            volume.uid = (new Date()).getTime();
            volume.status = {
                currentUsage: {
                    size: 0,
                    unit: 'B'
                },
                lastCapacityFirebreak: 0,
                lastPerformanceFirebreak: 0
            };
            
            volume.rate = 10000;
            volume.snapshots = [];
            
            if ( !angular.isDefined( volume.dataProtectionPolicy.snapshotPolicies ) ){
                volume.dataProtectionPolicy.snapshotPolicies = [];
            }
            
            // re-name the policies
            for ( var polIndex = 0; polIndex < volume.dataProtectionPolicy.snapshotPolicies.length; polIndex++ ){
                var policy = volume.dataProtectionPolicy.snapshotPolicies[polIndex];
                
                policy.uid = (new Date()).getTime() - polIndex;
                policy.type = volume.uid + '_SYSTEM_TIMELINE_' + policy.recurrenceRule.FREQ;
            }
            
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
                if ( volService.volumes[i].uid === volumeId ){
                    volume = volService.volumes[i];
                    break;
                }
            }
            
            if ( !angular.isDefined( volume.snapshots ) ){
                volume.snapshots = [];
            }
            
            var id = (new Date()).getTime();
            volume.snapshots.push( { uid: id, name: id, creation: id } );
            
            callback();
        };

        volService.deleteSnapshot = function( volumeId, snapshotId, callback, failure ){
            callback();
        };

        volService.getSnapshots = function( volumeId, callback, failure ){
            //callback( [] );
            
            for ( var i = 0; i < volService.volumes.length; i++ ){
                if ( volService.volumes[i].uid === volumeId ){
                    
                    if ( angular.isFunction( callback ) ){
                        callback( volService.volumes[i].snapshots );
                    }
                    
                    return volService.volumes[i].snapshots;
                }
            }
        };

        volService.getSnapshotPoliciesForVolume = function( volumeId, callback, failure ){
        //                callback( [] );
            var policies = [];
            
            for ( var i = 0; i < volService.volumes.length; i++ ){
                var volume = volService.volumes[i];
                
                if ( volume.uid === volumeId ){
                    policies = volume.dataProtectionPolicy.snapshotPolicies;
                    break;
                }
            }
            
            if ( angular.isFunction( callback ) ){
                callback( policies );
            }
            
            return {
                then: function( cb ){
                    if ( angular.isFunction( cb ) ){
                        cb( policies );
                    }
                }
            };
        };

        volService.getQosPolicyPresets = function( callback, failure ){
            
            var presets = [
                {
                    priority: 10,
                    iopsMin: 0,
                    iopsMax: 0,
                    id: 0,
                    name: 'Least Important'
                },
                {
                    priority: 7,
                    iopsMin: 0,
                    iopsMax: 0,
                    id: 1,
                    name: 'Standard'
                },
                {
                    priority: 1,
                    iopsMin: 0,
                    iopsMax: 0,
                    id: 2,
                    name: 'Most Important'
                }
            ];
                
            if ( angular.isFunction( callback ) ){
                callback( presets );
            }
        };
        
        volService.getDataProtectionPolicyPresets = function( callback, failure ){
            
            var presets = [{
                "id": 0,
                commitLogRetention: { 
                    "seconds": 86400,
                    "nanos" : 0
                },
                name: 'Sparse Coverage',
                snapshotPolicies: [{
                    type: 'SYSTEM_TIMELINE',
                    recurrenceRule: {
                        FREQ: 'DAILY',
                        BYMINUTE: ['0'],
                        BYHOUR: ['0']
                    },
                    retentionTime: {
                        seconds: 172800,
                        nanos: 0
                    }
                },
                {
                    type: 'SYSTEM_TIMELINE',
                    recurrenceRule: {
                        FREQ: 'WEEKLY',
                        BYMINUTE: ['0'],
                        BYHOUR: ['0'],
                        BYDAY: ['MO']
                    },
                    retentionTime: {
                        seconds: 604800,
                        nanos: 0
                    }
                },
                {
                    type: 'SYSTEM_TIMELINE',
                    recurrenceRule: {
                        FREQ: 'MONTHLY',
                        BYMINUTE: ['0'],
                        BYHOUR: ['0'],
                        BYMONTHDAY: ['1']
                    },
                    retentionTime: {
                        seconds: 7776000,
                        nanos: 0
                    }
                },
                {
                    type: 'SYSTEM_TIMELINE',
                    recurrenceRule: {
                        FREQ: 'YEARLY',
                        BYMINUTE: ['0'],
                        BYHOUR: ['0'],
                        BYMONTHDAY: ['1'],
                        BYMONTH: ['1']
                    },
                    retentionTime: {
                        seconds: 63244800,
                        nanos: 0
                    }
                }]
            },
            {
                id: 1,
                commitLogRetention: {
                    seconds: 86400,
                    nanos: 0
                },
                name: 'Standard',
                snapshotPolicies: [{
                    type: 'SYSTEM_TIMELINE',
                    recurrenceRule: {
                        FREQ: 'DAILY',
                        BYMINUTE: ['0'],
                        BYHOUR: ['0']
                    },
                    retentionTime: {
                        seconds: 604800,
                        nanos: 0
                    }
                },
                {
                    type: 'SYSTEM_TIMELINE',
                    recurrenceRule: {
                        FREQ: 'WEEKLY',
                        BYMINUTE: ['0'],
                        BYHOUR: ['0'],
                        BYDAY: ['MO']
                    },
                    retentionTime: {
                        seconds: 7776000,
                        nanos: 0
                    }
                },
                {
                    type: 'SYSTEM_TIMELINE',
                    recurrenceRule: {
                        FREQ: 'MONTHLY',
                        BYMINUTE: ['0'],
                        BYHOUR: ['0'],
                        BYMONTHDAY: ['1']
                    },
                    retentionTime: {
                        seconds: 15552000,
                        nanos: 0
                    }
                },
                {
                    type: 'SYSTEM_TIMELINE',
                    recurrenceRule: {
                        FREQ: 'YEARLY',
                        BYMINUTE: ['0'],
                        BYHOUR: ['0'],
                        BYMONTHDAY: ['1'],
                        BYMONTH: ['1']
                    },
                    retentionTime: {
                        seconds: 158112000,
                        nanos: 0
                    }
                }]
            },
            {
                id: 2,
                commitLogRetention: {
                    seconds: 172800,
                    nanos: 0
                },
                name: 'Dense Coverage',
                snapshotPolicies: [{
                    type: 'SYSTEM_TIMELINE',
                    recurrenceRule: {
                        FREQ: 'DAILY',
                        BYMINUTE: ['0'],
                        BYHOUR: ['0']
                    },
                    retentionTime: {
                        seconds: 2592000,
                        nanos: 0
                    }
                },
                {
                    type: 'SYSTEM_TIMELINE',
                    recurrenceRule: {
                        FREQ: 'WEEKLY',
                        BYMINUTE: ['0'],
                        BYHOUR: ['0'],
                        BYDAY: ['MO']
                    },
                    retentionTime: {
                        seconds: 18144000,
                        nanos: 0
                    }
                },
                {
                    type: 'SYSTEM_TIMELINE',
                    recurrenceRule: {
                        FREQ: 'MONTHLY',
                        BYMINUTE: ['0'],
                        BYHOUR: ['0'],
                        BYMONTHDAY: ['1']
                    },
                    retentionTime: {
                        seconds: 63244800,
                        nanos: 0
                    }
                },
                {
                    type: 'SYSTEM_TIMELINE',
                    recurrenceRule: {
                        FREQ: 'YEARLY',
                        BYMINUTE: ['0'],
                        BYHOUR: ['0'],
                        BYMONTHDAY: ['1'],
                        BYMONTH: ['1']
                    },
                    retentionTime: {
                        seconds: 474336000,
                        nanos: 0
                    }
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
        api.types = [
            {
                type: 'BLOCK',
                capacity: {
                    value: 10,
                    unit: 'GB'
                }
            },
            {
                type: 'OBJECT'
            }
        ];

        api.getVolumeTypes = function( callback ){
            callback( api.types );
        };

        return api;
    });
};

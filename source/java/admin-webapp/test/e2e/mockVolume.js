mockVol = function(){
    angular.module( 'volume-management', [] ).factory( '$volume_api', function(){

        var api = {};
        api.volumes = [];

        api.delete = function( volume ){

            var temp = [];

            api.volumes.forEach( function( v ){
                if ( v.name != volume.name ){
                    temp.push( v );
                }
            });

            api.volumes = temp;
        };

        api.save = function( volume, callback ){

            api.volumes.push( volume );

            if ( angular.isDefined( callback ) ){
                callback( volume );
            }
        };

        api.getSnapshots = function( volumeId, callback, failure ){
            //callback( [] );
        };

        api.getSnapshotPoliciesForVolume = function( volumeId, callback, failure ){
//                callback( [] );
        };

        api.refresh = function(){};

        return api;
    });
};

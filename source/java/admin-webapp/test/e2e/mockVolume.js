mockVol = function(){
    angular.module( 'volume-management', [] ).factory( '$volume_api', function(){

        var api = {};
        api.volumes = [];

        api.delete = function( volume ){

            for( var i = 0; i < api.volumes; i++ ){
                if ( volume.id === api.volumes[i].id ){
                    api.volumes.splice( i, 1 );
                    return;
                }
            }
        };

        api.save = function( volume ){
            api.volumes.push( volume );
        };


        return api;
    });
};

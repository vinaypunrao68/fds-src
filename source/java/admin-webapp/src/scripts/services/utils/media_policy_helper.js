angular.module( 'utility-directives' ).factory( '$media_policy_helper', ['$filter', function( $filter ){
    
    var service = {};
    
    service.availablePolicies = [
        { label: $filter( 'translate' )( 'volumes.tiering.l_ssd_only' ), value: 'SSD_ONLY' },
        { label: $filter( 'translate' )( 'volumes.tiering.l_hybrid' ), value: 'HYBRID_ONLY' },
        { label: $filter( 'translate' )( 'volumes.tiering.l_disk_only' ), value: 'HDD_ONLY' }
    ];
    
    service.convertRawToObjects = function( policy ){
        
        for ( var i = 0; i < service.availablePolicies.length; i++ ){
            if ( service.availablePolicies[i].value === policy ){
                return service.availablePolicies[i];
            }
        }
        
        return null;
    };
    
    return service;
}]);
angular.module( 'utility-directives' ).factory( '$media_policy_helper', ['$filter', '$http_fds', function( $filter, $http_fds ){
    
    var service = {};
    
    service.availablePolicies = [
//        { label: $filter( 'translate' )( 'volumes.tiering.l_ssd_only' ), value: 'SSD_ONLY' },
//        { label: $filter( 'translate' )( 'volumes.tiering.l_hybrid' ), value: 'HYBRID_ONLY' },
//        { label: $filter( 'translate' )( 'volumes.tiering.l_disk_only' ), value: 'HDD_ONLY' }
    ];
    
    service.convertRawToObjects = function( policy ){
        
        for ( var i = 0; i < service.availablePolicies.length; i++ ){
            if ( service.availablePolicies[i].value === policy ){
                return service.availablePolicies[i];
            }
        }
        
        return null;
    };
    
    service.getSystemCapabilities = function( callback ){
        
        $http_fds.get( '/api/config/system/capabilities', function( capabilities ){
                
                var tieringCapabilities = capabilities.supportedMediaPolicies;
                service.availablePolicies = [];
                
                for ( var i = 0; i < tieringCapabilities.length; i++ ){
                    
                    var type = tieringCapabilities[i];
                    
                    switch( type ){
                        case 'SSD_ONLY':
                            service.availablePolicies.push( { label: $filter( 'translate' )( 'volumes.tiering.l_ssd_only' ), value: 'SSD_ONLY' } );
                            break;
                        case 'HDD_ONLY':
                            service.availablePolicies.push( { label: $filter( 'translate' )( 'volumes.tiering.l_disk_only' ), value: 'HDD_ONLY' } );
                            break;
                        case 'HYBRID_ONLY':
                            service.availablePolicies.push( { label: $filter( 'translate' )( 'volumes.tiering.l_hybrid' ), value: 'HYBRID_ONLY' } );
                            break;
                    } // switch
                }// for
            
                if ( angular.isFunction( callback ) ){
                    callback( service.availablePolicies );       
                }
            }// then
        );
    };
    
    return service;
}]);
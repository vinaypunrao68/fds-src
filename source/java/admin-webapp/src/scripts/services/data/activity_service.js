angular.module( 'activity-management' ).factory( '$activity_service', [ '$http_fds', '$filter', function( $http_fds, $filter ){

    var service = {};
    
    service.FIREBREAK = 'FIREBREAk';
    service.PERFORMANCE = 'PERFORMANCE';
    service.VOLUMES = 'VOLUMES';
    service.STORAGE = 'STORAGE';
    service.SYSTEM = 'SYSTEM';
    
    service.getClass = function( category ){
        
        switch( category ){
            case service.FIREBREAK:
                return 'icon-firebreak';
            case service.PERFORMANCE:
                return 'icon-performance';
            case service.VOLUMES:
                return 'icon-volumes';
            case service.STORAGE:
                return 'icon-storage';
            case service.SYSTEM:
            default:
                return 'icon-nodes';
        }
    };

    service.getCategoryString = function( category ){
        
        switch( category ){
            case service.FIREBREAK:
                return $filter( 'translate' )( 'activities.l_firebreak' );
            case service.PERFORMANCE:
                return $filter( 'translate' )( 'activities.l_performance' );
            case service.VOLUMES:
                return $filter( 'translate' )( 'activities.l_volumes' );
            case service.STORAGE:
                return $filter( 'translate' )( 'activities.l_storage' );
            case service.SYSTEM:
            default:
                return $filter( 'translate' )( 'activities.l_system' );
        }
    };
    
    service.getActivities = function( filter, callback ){

//        return $http_fds.get( '/scripts/services/data/fakelogs.js',
//            function( response ){
//                callback( eval( response )[0] );
//            });
        
        return $http_fds.put( '/api/config/events', {points: 2}, callback );
    };

    return service;
}]);

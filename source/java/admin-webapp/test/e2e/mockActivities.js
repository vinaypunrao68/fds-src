mockActivities = function(){
    
    angular.module( 'activity-management' ).factory( '$activity_service', [ '$filter', function( $filter ){

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
            
            var events = JSON.parse( window.localStorage.getItem( 'activities' ));
            
            if ( !angular.isDefined( events ) || events === null ){
                events = [];
            }
            
            if ( angular.isDefined( filter.points ) && filter.points !== 0 ){
                var index = events.length - filter.points;
                
                if ( index > 0 ){
                    events = events.slice( index, filter.points );
                }
            }
            
            callback( {events: events} );
        };

        service.getSystemHealth = function( callback ){
            var rz = {
                "status": [
                    {
                    "state": "GOOD",
                    "category": "SERVICES",
                    "message": "l_services_good"
                    },
                    {
                    "state": "GOOD",
                    "category": "CAPACITY",
                    "message": "l_capacity_good"
                    },
                    {
                    "state": "GOOD",
                    "category": "FIREBREAK",
                    "message": "l_firebreak_good"
                    }
                ],
                "overall": "EXCELLENT"
            };
            
            callback( rz );
        };

        return service;
    }]);
};
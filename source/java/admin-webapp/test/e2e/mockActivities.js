mockActivities = function(){
    
    angular.module( 'activity-management' ).factory( '$activity_service', [ '$filter', '$stats_service', function( $filter, $stats_service ){

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
            
            callback( events );
        };
        
        var isActive = function( status ){
            
            if ( status === 'ACTIVE' ){
                return true;
            }
            else {
                return false;
            }
        };
        
        var determineServiceStatus = function(){
            
            var status = {
                state: 'GOOD',
                category: 'SERVICES',
                message: 'l_services_good'
            };
            
            if ( !angular.isDefined( window ) || !angular.isDefined( window.localStorage ) ){
                return status;
            }
            
            var nodes = window.localStorage.getItem( 'nodes' );
            
            if ( nodes === null ){
                return status;
            }
            
            nodes = JSON.parse( nodes );
            var allGood = true;
            var oneOm = false;
            var oneAm = false;
            
            for ( var i = 0; nodes !== null && i < nodes.length; i++ ){
                
                var node = nodes[i];
                
                var om = isActive( node.serviceMap.OM[0].status );
                var sm = isActive( node.serviceMap.SM[0].status );
                var dm = isActive( node.serviceMap.DM[0].status );
                var pm = isActive( node.serviceMap.PM[0].status );
                var am = isActive( node.serviceMap.AM[0].status );
                
                if ( om === sm === dm === pm === am === true ){
                    // all good... keep going.
                }
                else {
                    allGood = false;
                }
                
                if ( sm === false || dm === false || pm === false ){
                    status.state = 'BAD';
                    status.message = 'l_services_bad';
                    return status;
                }
                
                if ( om === true ){
                    oneOm = true;
                }
                
                if ( am === true ){
                    oneAm = true;
                }
            }
            
            if ( allGood === false ){

                status.state = 'OKAY';
                status.message = 'l_services_not_good';
            }
            
            return status;
        };
        
        var determineCapacityStatus = function(){
            
            var status = {
                state: 'GOOD',
                category: 'CAPACITY',
                message: 'l_capacity_good'
            };
            
            if ( !angular.isDefined( window ) || !angular.isDefined( window.localStorage ) ){
                return status;
            }
            
            var now = ((new Date()).getTime() / 1000).toFixed( 0 );
            var capSummary = $stats_service.getCapacitySummary( {contextList: [], range: {start: now - 24*60*60, end: now }, seriesType: [] } );
            
            var percentage = ((capSummary.calculated[1].total / capSummary.calculated[2]) * 100 ).toFixed( 0 );
            var timeToFull = capSummary.calculated[3].toFull;
            
            if ( timeToFull <= 7*24*60*60 ) {
                
                status.state = 'BAD';
                status.message = 'l_capacity_bad_rate';
                
            } else if ( percentage >= 90 ) {
                
                status.state = 'BAD';
                status.message = 'l_capacity_bad_threshold';
                
            } else if ( timeToFull <= 30*24*60*60 ) {
                
                status.state = 'OKAY';
                status.message = 'l_capacity_not_good_rate';
                
            } else if ( percentage >= 80 ) {
                
                status.state = 'OKAY';
                status.message = 'l_capacity_not_good_threshold';
                
            } else {
                
                status.state = 'GOOD';
                status.message = 'l_capacity_good';
            }
            
            return status;
        };
        
        var determineFirebreakStatus = function(){
            
            var status = {
                state: 'GOOD',
                category: 'FIREBREAK',
                message: 'l_firebreak_good'
            };
            
            if ( !angular.isDefined( window ) || !angular.isDefined( window.localStorage ) ){
                return status;
            }
            
            var volumes = window.localStorage.getItem( 'volumes' );
            
            if ( volumes === null ){
                return status;
            }
            
            volumes = JSON.parse( volumes );
            
            var now = (new Date()).getTime() / 1000;
            var fbSummary = $stats_service.getFirebreakSummary( { contextList: [], range: { start: now - 24*60*60, end: now }, seriesType: []} );
            
            var fbCount = parseInt( fbSummary.calculated[0].count );
            var percentage = (( fbCount / volumes.length ) * 100 ).toFixed( 0 );
            
            if ( percentage > 50 ){
                status.state = 'BAD';
                status.message = 'l_firebreak_bad';
            }
            else if ( fbCount > 0 ){
                status.state = 'OKAY';
                status.message = 'l_firebreak_not_good';
            }
            
            return status;
        };
        
        var getPointsForState = function( state ){

            var points = 0;

            switch (state) {
                case 'OKAY':
                    points = 1;
                    break;
                case 'BAD':
                    points = 2;
                    break;
                default:
                    break;
            }

            return points;   
        };

        service.getSystemHealth = function( callback ){
            
            var serviceStatus = determineServiceStatus();
            var capacityStatus = determineCapacityStatus();
            var firebreakStatus = determineFirebreakStatus();
            
            var overallStatus = 'GOOD';

            // if services are bad - system is bad.
            if ( serviceStatus.state === 'BAD' ) {
                overallStatus = 'LIMITED';
            }
            // if capacity is bad, system is marginal
            else if ( capacityStatus.state === 'BAD' ) {
                overallStatus = 'MARGINAL';
            }

            // now that the immediate status are done, the rest is determined
            // by how many areas are in certain conditions.  We'll do it
            // by points.  okay = 1pt, bad = 2pts.  
            //
            // good is <= 1pts.  accetable <=3 marginal > 3
            var points = 0;
            points += getPointsForState( serviceStatus.state );
            points += getPointsForState( capacityStatus.state );
            points += getPointsForState( firebreakStatus.state );

            if (points == 0) {
                overallStatus = 'EXCELLENT';
            } else if (points == 1) {
                overallStatus = 'GOOD';
            } else if (points <= 3) {
                overallStatus = 'ACCEPTABLE';
            } else if (points < 6) {
                overallStatus = 'MARGINAL';
            } else {
                overallStatus = 'LIMITED';
            }          
            
            var rz = {
                status: [
                    serviceStatus,
                    capacityStatus,
                    firebreakStatus
                ],
                overall: overallStatus
            };
            
            callback( rz );
        };

        return service;
    }]);
};
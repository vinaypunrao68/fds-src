angular.module( 'status' ).controller( 'statusController', ['$scope', '$activity_service', '$interval', '$authorization', '$stats_service', '$filter', '$interval', function( $scope, $activity_service, $interval, $authorization, $stats_service, $filter, $interval ){

//    $scope.fakeData = {
//        series: [
//          [
//              {x:0, y: 10},
//              {x:1, y: 40},
//              {x:2, y: 31},
//              {x:3, y: 16},
//              {x:4, y: 19},
//              {x:5, y: 2},
//              {x:6, y: 8},
//              {x:7, y: 52},
//              {x:8, y: 12},
//              {x:9, y: 34}
//          ],
//          [
//              {x:0, y: 15},
//              {x:1, y: 46},
//              {x:2, y: 36},
//              {x:3, y: 21},
//              {x:4, y: 30},
//              {x:5, y: 12},
//              {x:6, y: 10},
//              {x:7, y: 53},
//              {x:8, y: 17},
//              {x:9, y: 39}
//          ]
//        ]
//    };
//    
//    $scope.fakeColors = [ 'red', 'red' ];
//    $scope.fakeOpacities = [0.5, 0.5];
    
    $scope.activities = [];
    $scope.firebreakMax = 1440;
    $scope.firebreakStats = { series: [[]], summaryData: { hoursSinceLastEvent: 0 }};
    
    $scope.activitiesReturned = function( list ){
        $scope.activities = eval(list);
    };
    
    $scope.firebreakReturned = function( data ){
        $scope.firebreakStats = data;
    };
    
    $scope.setFirebreakToolipText = function( data ){
        
        var units = '';
        var value = 0;
        
        // set the text as days
        if ( data.secondsSinceLastFirebreak > 24*60*60 ){
            value = Math.floor( data.secondsSinceLastFirebreak / (24*60*60) )
            units = $filter( 'translate' )( 'common.days' );
        }
        // set the text as hours
        else if ( data.secondsSinceLastFirebreak > (60*60) ){
            value = Math.floor( data.secondsSinceLastFirebreak / (60*60) )
            units = $filter( 'translate' )( 'common.hours' );            
        }
        else {
            value = Math.floor( data.secondsSinceLastFirebreak / 60 );
            units = $filter( 'translate' )( 'common.minutes' );
        }
        
        var str = '<div><div style="font-weight: bold;font-size: 11px;">' + data.name + '</div><div style="font-size: 10px;">' + $filter( 'translate' )( 'status.tt_firebreak', { value: value,units: units} ) + '</div></div>';
        
        console.log(  );
        
        return str;
    };

    $scope.isAllowed = function( permission ){
        var isit = $authorization.isAllowed( permission );
        return isit;
    };

    // pollers
    var firebreakInterval = $interval( function(){ $stats_service.getFirebreakSummary( $scope.firebreakReturned );}, 10000 );
    
    $activity_service.getActivities( '', '', 30, $scope.activitiesReturned );
    
    // cleanup the pollers
    $scope.$on( '$destroy', function(){
        $interval.cancel( firebreakInterval );
    });
    
    $stats_service.getFirebreakSummary( $scope.firebreakReturned );

}]);

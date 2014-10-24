angular.module( 'base' ).factory( '$time_converter', [ '$filter', function( $filter ){
    
    var service = {};
    
    service.MS_PER_SECOND = 1000;
    service.MS_PER_MINUTE = service.MS_PER_SECOND*60;
    service.MS_PER_HOUR = service.MS_PER_MINUTE*60;
    service.MS_PER_DAY = service.MS_PER_HOUR*24;
    service.MS_PER_WEEK = service.MS_PER_DAY*7;
    service.MS_PER_4_WEEKS = service.MS_PER_WEEK * 4;
    
    var secondsAgo = $filter( 'translate' )( 'common.l_seconds_ago' );
    var minutesAgo = $filter( 'translate' )( 'common.l_minutes_ago' );
    var hoursAgo = $filter( 'translate' )( 'common.l_hours_ago' );
    var daysAgo = $filter( 'translate' )( 'common.l_days_ago' );
    var weeksAgo = $filter( 'translate' )( 'common.l_weeks_ago' );
    var yesterday = $filter( 'translate' )( 'common.l_yesterday' );
    
    service.convertToTimePastLabel = function( ms ){
        var timePast = (new Date()).getTime() - ms;
        
        if ( timePast < service.MS_PER_MINUTE ){
            return Math.round( timePast / service.MS_PER_SECOND ) + ' ' + secondsAgo;
        }
        else if ( timePast < service.MS_PER_HOUR ){
            return Math.round( timePast / service.MS_PER_MINUTE ) + ' ' + minutesAgo;
        }
        else if ( timePast < service.MS_PER_DAY ){
            return Math.round( timePast / service.MS_PER_HOUR ) + ' ' + hoursAgo;
        }
        else if ( timePast < service.MS_PER_WEEK ){
            
            var val = Math.round( timePast / service.MS_PER_DAY );
            
            if ( val === 1 ){
                return yesterday;
            }
            else {
                return Math.round( timePast / service.MS_PER_DAY ) + ' ' + daysAgo;
            }
        }
        else if ( timePast < service.MS_PER_4_WEEKS ){
            return Math.round( timePast / service.MS_PER_WEEK ) + ' ' + weeksAgo;
        }
        else {
            return (new Date( ms )).toLocaleDateString();
        }
    };
    
};
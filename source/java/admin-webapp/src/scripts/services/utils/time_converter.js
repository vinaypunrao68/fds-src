angular.module( 'base' ).factory( '$time_converter', [ '$filter', function( $filter ){
    
    var service = {};
    
    service.MILLIS = 0;
    service.SECONDS = 1;
    service.MINUTES = 2;
    service.HOURS = 3;
    service.DAYS = 4;
    service.WEEKS = 5;
    service.YEARS = 6;
    
    service.MS_PER_SECOND = 1000;
    service.MS_PER_MINUTE = service.MS_PER_SECOND*60;
    service.MS_PER_HOUR = service.MS_PER_MINUTE*60;
    service.MS_PER_DAY = service.MS_PER_HOUR*24;
    service.MS_PER_WEEK = service.MS_PER_DAY*7;
    service.MS_PER_MONTH = service.MS_PER_DAY * 30;
    service.MS_PER_YEAR = service.MS_PER_DAY*366;
    
    var secondsAgo = $filter( 'translate' )( 'common.l_seconds_ago' );
    var minutesAgo = $filter( 'translate' )( 'common.l_minutes_ago' );
    var hoursAgo = $filter( 'translate' )( 'common.l_hours_ago' );
    var daysAgo = $filter( 'translate' )( 'common.l_days_ago' );
    var weeksAgo = $filter( 'translate' )( 'common.l_weeks_ago' );
    var yesterday = $filter( 'translate' )( 'common.l_yesterday' );
    
    var pluralize = function( value, plural, singular ){

        if ( value === 1 ){
            return singular;
        }
        else {
            return plural;
        }
    };
    
    service.convertToTime = function( ms, fixed ){
        
        var value = 0;
        var unit = service.MILLIS;
        var str = '';
        
        if ( ms < service.MS_PER_SECOND ){
            value = ms;
            unit = service.MILLIS;
        }
        else if ( ms < service.MS_PER_MINUTE ){
            value = Math.round( ms / service.MS_PER_SECOND );
            unit = service.SECONDS;
        }
        else if ( ms < service.MS_PER_HOUR ){
            value = Math.round( ms / service.MS_PER_MINUTE );
            unit = service.MINUTES;
        }
        else if ( ms < service.MS_PER_DAY ){
            value = Math.round( ms / service.MS_PER_HOUR );
            unit = service.HOURS;
        }
        else if ( ms < service.MS_PER_WEEK ){
            value = Math.round( ms / service.MS_PER_DAY );
            unit = service.DAYS;
        }
        else if ( ms < service.MS_PER_MONTH ){
            value = Math.round( ms / service.MS_PER_WEEK );
            unit = service.WEEKS;
        }
        else if ( ms < service.MS_PER_YEAR ){
            value = 30*Math.round( ms / service.MS_PER_MONTH );
            unit = service.DAYS;
        }
        else {
            value = Math.round( ms / service.MS_PER_YEAR );
            unit = service.YEARS;
        }
        
        if ( !angular.isNumber( fixed ) ){
            fixed = 2;
        }
        
        str += value.toFixed( fixed ) + ' ';
        
        switch( unit ){
            case service.MILLIS:
                str += pluralize( value, $filter( 'translate' )( 'common.l_millis' ), $filter( 'translate' )( 'common.l_milli' ) );
                break;
            case service.SECONDS:
                str += pluralize( value, $filter( 'translate' )( 'common.l_seconds' ), $filter( 'translate' )( 'common.l_second' ) );
                break;
            case service.MINUTES:
                str += pluralize( value, $filter( 'translate' )( 'common.l_minutes' ), $filter( 'translate' )( 'common.l_minute' ) );
                break;
            case service.HOURS:
                str += pluralize( value, $filter( 'translate' )( 'common.l_hours' ), $filter( 'translate' )( 'common.l_hour' ) );
                break;
            case service.DAYS:
                str += pluralize( value, $filter( 'translate' )( 'common.l_days' ), $filter( 'translate' )( 'common.l_day' ) );
                break;                
            case service.WEEKS:
                str += pluralize( value, $filter( 'translate' )( 'common.l_weeks' ), $filter( 'translate' )( 'common.l_week' ) );
                break;                
            default:
                str += pluralize( value, $filter( 'translate' )( 'common.l_years' ), $filter( 'translate' )( 'common.l_year' ) );
                break;                                
        }
        
        return str;
    };
    
    service.convertToTimePastLabel = function( ms ){
        var timePast = (new Date()).getTime() - ms;
        
        var value = 0;
        var unit = service.MILLIS;
        
        if ( timePast < service.MS_PER_MINUTE ){
            value = Math.round( timePast / service.MS_PER_SECOND );
            unit = service.SECONDS;
        }
        else if ( timePast < service.MS_PER_HOUR ){
            value = Math.round( timePast / service.MS_PER_MINUTE );
            unit = service.MINUTES;
        }
        else if ( timePast < service.MS_PER_DAY ){
            value = Math.round( timePast / service.MS_PER_HOUR );
            unit = service.HOURS;
        }
        else if ( timePast < service.MS_PER_WEEK ){
            
            var val = Math.round( timePast / service.MS_PER_DAY );
            
            if ( val === 1 ){
                return yesterday;
            }
            else {
                value = Math.round( timePast / service.MS_PER_DAY );
                unit = service.DAYS;
            }
        }
        else if ( timePast < service.MS_PER_4_WEEKS ){
            value = Math.round( timePast / service.MS_PER_WEEK );
            unit = service.WEEKS;
        }
        else {
            return (new Date( ms )).toLocaleDateString();
        }
        
        var str = value + ' ';
        
        switch( unit ){
            case service.SECONDS:
                str += pluralize( value, $filter( 'translate' )( 'common.l_seconds_ago' ), $filter( 'translate' )( 'common.l_second_ago' ) );
                break;
            case service.MINUTES:
                str += pluralize( value, $filter( 'translate' )( 'common.l_minutes_ago' ), $filter( 'translate' )( 'common.l_minute_ago' ) );
                break;
            case service.HOURS:
                str += pluralize( value, $filter( 'translate' )( 'common.l_hours_ago' ), $filter( 'translate' )( 'common.l_hour_ago' ) );
                break;
            case service.DAYS:
                str += pluralize( value, $filter( 'translate' )( 'common.l_days_ago' ), $filter( 'translate' )( 'common.l_day_ago' ) );
                break;                
            case service.WEEKS:
                str += pluralize( value, $filter( 'translate' )( 'common.l_weeks_ago' ), $filter( 'translate' )( 'common.l_week_ago' ) );
                break;                
            default:
                str += $filter( 'translate' )( 'common.l_now' );
                break;                                
        }
        
        return str;
    };
    
    return service;
}]);
var convertTimePeriodToSeconds = function( value, timeUnit ){

    var mult;

    switch( timeUnit ){
        case 'HOURLY':
            mult = 60*60;
            break;
        case 'DAILY':
            mult = 24*60*60;
            break;
        case 'WEEKLY':
            mult = 7*24*60*60;
            break;
        case 'MONTHLY':
            mult = 31*24*60*60;
            break;
        case 'YEARLY':
            mult = 366*24*60*60;
            break;
        default:
            mult = 60*60;
    }

    return (value*mult);
};

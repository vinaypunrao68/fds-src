angular.module( 'form-directives' ).directive( 'timeSpinner', function(){
    
    return {
        
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/widgets/timespinner/timespinner.html',
        scope: { time: '=ngModel', showSeconds: '@', mode: '@', min: '=?', max: '=?', showDate: '@', separator: '@' },
        controller: function( $scope, $timeout ){
            
            $scope.hours = 0;
            $scope.minutes = 0;
            $scope.seconds = 0;
            $scope.months = 1;
            $scope.days = 1;
            $scope.years = 2000;
            
            $scope.HOURS = 0;
            $scope.MINUTES = 1;
            $scope.SECONDS = 2;
            $scope.AMPM = 3;
            $scope.MONTHS = 4;
            $scope.YEARS = 5;
            $scope.DAYS = 6;
            
            var AM = 'a.m.';
            var PM = 'p.m.';
            
            $scope.ampm = AM;
            
            if ( !angular.isDefined( $scope.mode ) || $scope.mode != 24 ){
                $scope.mode = 12;
            }
            
            if ( !angular.isDefined( $scope.showDate ) ){
                $scope.showDate = false;
            }
            
            if ( !angular.isDefined( $scope.separator ) ){
                $scope.separator = 'at';
            }
            
            var convertHours = function( hour ){
                
                hour = hour % 12;
                
                if ( hour === 0 ){
                    hour = 12;
                }
                
                return hour;
            };
            
            var pad = function( digit ){
                if ( digit < 10 && (digit+'').length === 1 ){
                    digit = '0' + digit;
                }
                
                return digit;
            };
            
            // rationalize the pieces into an actual date.
            var fixDate = function( months, days, years, hours, minutes, seconds, ampm ){
            
                var unblank = function( val ){
                    if ( val === '' || !angular.isNumber( val ) ){
                        return 0;
                    }
                    
                    return val;
                };
                
                months = unblank( months );
                days = unblank( days );
                years = unblank( years );
                hours = unblank( hours );
                minutes = unblank( minutes );
                seconds = unblank( seconds );
                
                var hourLimits = { min: 0, max: 23 };
                var minuteLimits = { min: 0, max: 59 };
                var secondLimits = { min: 0, max: 59 };
                var monthLimits = { min: 0, max: 11 };
                var dayLimits = { min: 1, max: 31 };
                var yearLimits = { min: 1969, max: 9999 };
                
                if ( angular.isDefined( $scope.min ) ){
                    hourLimits.min = $scope.min.getHours();
                    minuteLimits.min = $scope.min.getMinutes();
                    secondLimits.min = $scope.min.getSeconds();
                    monthLimits.min = $scope.min.getMonth();
                    dayLimits.min = $scope.min.getDate();
                    yearLimits.min = $scope.min.getFullYear();
                }
                
                if ( angular.isDefined( $scope.max ) ){
                    hourLimits.max = $scope.max.getHours();
                    minuteLimits.max = $scope.max.getMinutes();
                    secondLimits.max = $scope.max.getSeconds();
                    monthLimits.max = $scope.max.getMonth();
                    dayLimits.max = $scope.max.getDate();
                    yearLimits.max = $scope.max.getFullYear(); 
                }
                
                // make hours 24
                if ( $scope.mode == 12 && ampm === PM ){
                    
                    if ( hours === 12 ){
                        hours = 0;
                    } 
                    
                    if ( ampm === PM ){
                        
                        hours += 12;
                    }
                }
                
                var fixRange = function( value, limits ){
                    if ( value < limits.min ){
                        value = limits.min;
                    }
                    else if ( value > limits.max ){
                        value = limits.max;
                    }
                    
                    return value;
                };
                
                hours = fixRange( hours, hourLimits );
                minutes = fixRange( minutes, minuteLimits );
                seconds = fixRange( seconds, secondLimits );
                months = fixRange( months, monthLimits );
                
                var tDate = new Date();
                tDate.setFullYear( years );
                tDate.setMonth( months + 1 );
                tDate.setDate( 0 );
                dayLimits.max = tDate.getDate();
                
                days = fixRange( days, dayLimits );
                years = fixRange( years, yearLimits );
                
                var viewHours = hours;
                
                // fix for 12 hour mode
                if ( $scope.mode == 12 ){
                    if ( viewHours > 11 ){
                        ampm = PM;
                        
                        viewHours = viewHours % 12;
                        
                        if ( viewHours === 0 ){
                            viewHours = 12;
                        }
                    }
                    else {
                        ampm = AM;
                        
                        if ( viewHours === 0 ){
                            viewHours = 12;
                        }
                    }
                }
                
                var date = new Date();
                date.setHours( hours );
                date.setMinutes( minutes );
                date.setSeconds( seconds );
                date.setFullYear( years );
                date.setMonth( months );
                date.setDate( days );
                
                $scope.hours = pad( viewHours );
                $scope.minutes = pad( minutes );
                $scope.seconds = pad( seconds );
                $scope.months = pad( months+1 );
                $scope.days = pad( days );
                $scope.years = years;
                $scope.ampm = ampm;
                
                if ( $scope.time.toString() !== date.toString() ){
                    $scope.time = date;
                }
            };
            
            $scope.focusChanged = function(){
                
               fixDate( parseInt( $scope.months ), parseInt( $scope.days ), parseInt( $scope.years ), parseInt( $scope.hours ), parseInt( $scope.minutes ), parseInt( $scope.seconds ), $scope.ampm );
            };
            
            $scope.up = function( BOX ){
                
                var mon = $scope.months;
                var day = $scope.days;
                var year = $scope.years;
                var h = $scope.hours;
                var m = $scope.minutes;
                var s = $scope.seconds;
                
                switch( BOX ){
                    case $scope.HOURS:
                        h = parseInt( $scope.hours ) + 1;
                        break;
                    case $scope.MINUTES:
                        m = parseInt( $scope.minutes ) + 1;
                        break;
                    case $scope.SECONDS:
                        s = parseInt( $scope.seconds ) + 1;
                        break;
                    case $scope.AMPM: 
                        if ( $scope.ampm === AM ){
                            $scope.ampm = PM;
                        }
                        else {
                            $scope.ampm = AM;
                        }
                        break;
                    case $scope.MONTHS:
                        mon = parseInt( $scope.months );
                        break;
                    case $scope.YEARS:
                        year = parseInt( $scope.years ) + 1;
                        break;
                    case $scope.DAYS:
                        day = parseInt( $scope.days ) + 1;
                        break;
                    default:
                        break;
                        
                }
                
                fixDate( mon, day, year, h, m, s, $scope.ampm );
            };
            
            $scope.down = function( BOX ){

                var mon = $scope.months;
                var day = $scope.days;
                var year = $scope.years;
                var h = $scope.hours;
                var m = $scope.minutes;
                var s = $scope.seconds;
                
                switch( BOX ){
                    case $scope.HOURS: 
                        var h = parseInt( $scope.hours ) - 1;
                        break;
                    case $scope.MINUTES:
                        var m = parseInt( $scope.minutes ) - 1;
                        break;
                    case $scope.SECONDS:
                        var s = parseInt( $scope.seconds ) - 1;
                        break;
                    case $scope.AMPM:
                        if ( $scope.ampm === AM ){
                            $scope.ampm = PM;
                        }
                        else {
                            $scope.ampm = AM;
                        }
                        break;
                    case $scope.MONTHS:
                        mon = parseInt( $scope.months ) - 2;
                        break;
                    case $scope.YEARS:
                        year = parseInt( $scope.years ) - 1;
                        break;
                    case $scope.DAYS:
                        day = parseInt( $scope.days ) - 1;
                        break;                        
                    default:
                        break;
                }
                
                fixDate( mon, day, year, h, m, s, $scope.ampm );
            };
            
            $scope.keyPressed = function( $event, BOX ){
                
                if ( $event.keyCode === 38 ){
                    $scope.up( BOX );
                }
                else if ( $event.keyCode === 40 ){
                    $scope.down ( BOX );
                }
            };
            
            $scope.$watch( 'time', function( newVal, oldVal ){
                
                if ( newVal === oldVal && ($scope.hours !== 0 && $scope.minutes !== 0 && $scope.seconds !== 0 ) ){
                    return;
                }
                
                if ( angular.isNumber( newVal ) ){
                    newVal = new Date( newVal );
                }
                
                if ( !angular.isDate( newVal ) ){
                    $scope.hours = 0;
                    $scope.minutes = 0;
                    $scope.seconds = 0;
                    $scope.months = 1;
                    $scope.days = 1;
                    $scope.years = 2000;
                    return;
                }
                
                var ampm = (newVal.getHours() >= 12) ? PM : AM;
                fixDate( newVal.getMonth(), newVal.getDate(), newVal.getFullYear(), newVal.getHours(), newVal.getMinutes(), newVal.getSeconds(), ampm );
            });
        }
    };
});
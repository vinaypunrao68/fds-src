angular.module( 'form-directives' ).directive( 'timeSpinner', function(){
    
    return {
        
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/widgets/timespinner/timespinner.html',
        scope: { time: '=ngModel', showSeconds: '@', mode: '@' },
        controller: function( $scope ){
            
            $scope.hours = 0;
            $scope.minutes = 0;
            $scope.seconds = 0;
            
            $scope.HOURS = 0;
            $scope.MINUTES = 1;
            $scope.SECONDS = 2;
            $scope.AMPM = 3;
            
            var AM = 'a.m.';
            var PM = 'p.m.';
            
            $scope.ampm = AM;
            
            if ( !angular.isDefined( $scope.mode ) || $scope.mode != 24 ){
                $scope.mode = 12;
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
            
            var fix = function( value, min, max ){
                
                if ( value < min ){
                    return min;
                }
                
                if ( value > max ){
                    return max;
                }
                
                return value;
            };
            
            var createDate = function( newVal ){
                
                if ( angular.isDefined( newVal ) ){
                    $scope.hours = newVal.getHours();
                    $scope.minutes = newVal.getMinutes();
                    $scope.seconds = newVal.getSeconds();
                    
                    if ( $scope.hours > 11 ){
                        $scope.ampm = PM;
                    }
                    else {
                        $scope.ampm = AM;
                    }
                }
                
                if ( $scope.mode == 12 ){
                    $scope.hours = convertHours ( $scope.hours );
                }
                
                if ( $scope.showSeconds == 'true' ){
                    $scope.seconds = newVal.getSeconds();
                    $scope.seconds = pad( $scope.seconds );
                }
                
                $scope.hours = pad( $scope.hours );
                $scope.minutes = pad( $scope.minutes );
            };
            
            $scope.focusChanged = function(){
                
                // validate the numbers
                var h = parseInt( $scope.hours );
                
                // fix hours
                if ( $scope.mode == 12 ){
                    
                    if ( $scope.ampm === PM ){
                        h += 12;
                        if ( h === 24 ){
                            h = 0;
                        }
                    }
                    else {
                        if ( h === 12 ){
                            h = 0;
                        }
                    }
                }
                
                //fix minutes
                var m = parseInt( $scope.minutes );
                m = fix( m, 0, 59 );
                
                var s = parseInt( $scope.seconds );
                s = fix( s, 0, 59 );
                
                var date = new Date();
                date.setHours( h );
                date.setMinutes( m );
                date.setSeconds( s );
                
                $scope.time = date;
            };
            
            $scope.up = function( BOX ){
                
                switch( BOX ){
                    case $scope.HOURS:
                        var h = parseInt( $scope.hours ) + 1;
                        
                        if ( $scope.mode == 12 ){
                            
                            if ( h == 12 ){
                                if ( $scope.ampm === AM ){
                                    $scope.ampm = PM;
                                }
                                else {
                                    $scope.ampm = AM;
                                }
                            }
                            
                            if ( h >= 13 ){
                                h = 1;
                            }
                        }
                        else {
                            if ( h > 23 ){
                                h = 0;
                            }
                        }
                        
                        $scope.hours = h;
                        
                        break;
                    case $scope.MINUTES:
                        var m = parseInt( $scope.minutes ) + 1;
                        
                        if ( m > 59 ){
                            m = 0;
                        }
                        
                        $scope.minutes = m;
                        
                        break;
                    case $scope.SECONDS:
                        var s = parseInt( $scope.seconds ) + 1;
                        
                        if ( s > 59 ){
                            s = 0;
                        }
                        
                        $scope.seconds = s;
                        
                        break;
                    case $scope.AMPM: 
                        if ( $scope.ampm === AM ){
                            $scope.ampm = PM;
                        }
                        else {
                            $scope.ampm = AM;
                        }
                    default:
                        break;
                        
                }
                
                createDate();
            };
            
            $scope.down = function( BOX ){
                
                switch( BOX ){
                    case $scope.HOURS: 
                        var h = parseInt( $scope.hours ) - 1;
                        
                        if ( $scope.mode == 12 ){
                            
                            if ( h <= 0 ){
                                h = 12;
                                
                                if ( $scope.ampm === AM ){
                                    $scope.ampm = PM;
                                }
                                else {
                                    $scope.ampm = AM;
                                }
                            }
                        }
                        else {
                            if ( h < 0 ){
                                h = 23;
                            }
                        }
                        
                        $scope.hours = h;
                        
                        break;
                    case $scope.MINUTES:
                        var m = parseInt( $scope.minutes ) - 1;
                        if ( m < 0 ){
                            m = 59;
                        }
                        
                        $scope.minutes = m;
                        
                        break;
                    case $scope.SECONDS:
                        var s = parseInt( $scope.seconds ) - 1;
                        if ( s < 0 ){
                            s = 59;
                        }
                        
                        $scope.seconds = s;
                        
                        break;
                    case $scope.AMPM:
                        if ( $scope.ampm === AM ){
                            $scope.ampm = PM;
                        }
                        else {
                            $scope.ampm = AM;
                        }
                        break;
                    default:
                        break;
                }
                
                createDate();
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
                
                if ( !angular.isDate( newVal ) ){
                    $scope.hours = 0;
                    $scope.minutes = 0;
                    $scope.seconds = 0;
                    return;
                }
                
                createDate( newVal );
            });
        }
    };
});
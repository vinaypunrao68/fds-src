angular.module( 'form-directives' ).directive( 'dateTimeTyper', function(){
    
    return {
        restrict: 'E', 
        replace: true,
        transclude: true,
        templateUrl: 'scripts/directives/widgets/datetimetyper/datetimetyper.html',
        scope: { date: '=ngModel', mode: '@' },
        controller: function( $scope, $element, $timeout ){
            
            $scope.editing = false;
            $scope.pos = 0;
            
            var pos = 0;
            var min = 0;
            var max = 0;
            
            var pad = function( val ){
                
                val += '';
                
                if ( val.length === 1 ){
                    val = '0' + val;
                }
                
                return val;
            };
            
            var getInput = function(){
                return $element.find( 'input' )[0];
            };
            
            var setText = function(){
                
                if ( !angular.isDefined( $scope.date ) ){
                    return;
                }
                
                var date = $scope.date;
                
                if ( !angular.isDate( $scope.date ) ){
                    date = new Date( $scope.date );
                }
                
                var s = pad( date.getMonth()+1 ) + '/' + pad( date.getDate() ) + '/' +
                    pad( date.getFullYear() ) + ' ';
                
                if ( $scope.mode == 12 ){
                    
                    var h = date.getHours() % 12;
                    
                    if ( h === 0 ){
                        h = 12;
                    }
                    
                    s += pad( h );
                }
                else {
                    s += pad( date.getHours() );
                }
                
                s += ':' + pad( date.getMinutes() );
                
                
                if ( $scope.mode == 12 ){
                    if ( date.getHours > 11 ){
                        s += 'PM';
                    }
                    else {
                        s +=' AM';
                    }
                }
                
                $scope.rawvalue = s;
            };
            
            var getValue = function( start, end ){
                
                var v = $scope.rawvalue.substring( start, end );
                v = parseInt( v );
                
                return v;
            };
            
            var setSelection = function( pos ){
                var input = getInput();
                var start = 0;
                var end = 0;
                
                // month
                if ( pos <= 2 ){
                    start = 0;
                    end = 2;
                }
                // day
                else if ( pos <= 5 ){
                    start = 3;
                    end = 5;
                }
                // year
                else if ( pos <= 10 ){
                    start = 6;
                    end = 10;
                }
                // hour
                else if ( pos <= 13 ){
                    start = 11;
                    end = 13;
                }
                // minute
                else if ( pos <= 16 ){
                    start = 14;
                    end = 16;
                }
                // ampm
                else if ( pos < 19 ){
                    start = 17;
                    end = 19;
                }
                
                input.selectionStart = start;
                input.selectionEnd = end;
            };
            
            $scope.fieldClicked = function( $event ){
                var input = getInput();
                pos = input.selectionStart;
                
                setSelection( pos );
            };
            
            var fixBadField = function( val ){
                
                var fStr = '';
                
                for ( var i = 0; i < val.length; i++ ){
                    
                    var rez = parseInt( val[i] );
                    
                    if ( isNaN( rez )){
                        continue;
                    }
                    
                    fStr += rez;
                }
                
                return fStr;
            };
            
            var validateFields = function( pos ){
                
                // is the month right
                var m = fixBadField( $scope.rawvalue.substr( 0, 2 ) );
                
                if ( m < 1 ){
                    m = 1;
                }
                else if ( m > 12 ){
                    m = 12;
                }
                
                $scope.rawvalue = pad( m ) + '/' + $scope.rawvalue.substr( 3 );
                
                // year
                var y = fixBadField( $scope.rawvalue.substr( $scope.rawvalue.lastIndexOf( '/' ) + 1, 4 ) );
                if ( y < 1000 ){
                    y = 1000;
                }
                
                $scope.rawvalue = $scope.rawvalue.substring( 0, $scope.rawvalue.lastIndexOf( '/' ) + 1 ) + y + $scope.rawvalue.substr( $scope.rawvalue.indexOf( ' ' ) );
                
                // day
                var d = fixBadField( $scope.rawvalue.substr( $scope.rawvalue.indexOf( '/' ) + 1, 2 ) );
                var tDate = new Date();
                tDate.setFullYear( y );
                tDate.setMonth( m  );
                tDate.setDate( 0 );
                
                if ( d < 1 ){
                    d = 1;
                }
                else if ( d > tDate.getDate() ){
                    d = tDate.getDate();
                }
                
                $scope.rawvalue = $scope.rawvalue.substring( 0, $scope.rawvalue.indexOf( '/' ) + 1 ) + pad( d ) + $scope.rawvalue.substr( $scope.rawvalue.lastIndexOf( '/' ) );
                
                //hour
                var h = fixBadField( $scope.rawvalue.substr( $scope.rawvalue.indexOf( ' ' ) + 1, 2 ) );
                var ampm;
                
                if ( h < 0 ){
                    h = 0;
                }
                else if ( h > 23 ){
                    h = 23;
                }
                
                if ( $scope.mode == 12 ){
                    
                    if ( h > 11 ){
                        ampm = 'PM';
                    }
                    else {
                        ampm = 'AM';
                    }
                    
                    h = h % 12;
                    
                    if ( h === 0 ){
                        h = 12;
                    }
                }
                
                $scope.rawvalue = $scope.rawvalue.substring( 0, $scope.rawvalue.indexOf( ' ' )+1 ) + pad( h ) + $scope.rawvalue.substr( $scope.rawvalue.indexOf( ':' ) );
                
                //minutes
                var min = fixBadField( $scope.rawvalue.substr( $scope.rawvalue.indexOf( ':' ) + 1, 2 ) );
                if ( min < 0 ){
                    min = 0;
                }
                else if ( min > 59 ){
                    min = 59;
                }
                
                $scope.rawvalue = $scope.rawvalue.substring( 0, $scope.rawvalue.indexOf( ':' ) + 1) + pad( min ) + $scope.rawvalue.substr( $scope.rawvalue.lastIndexOf( ' ' ) );
                
                // ampm
                var a = $scope.rawvalue.substr( $scope.rawvalue.length - 2 ).trim();
                
                if ( !angular.isDefined( ampm ) || a.length === 1 ){
                    
                    if ( a.charAt( 0 ).toLowerCase() === 'a' ){
                        ampm = 'AM';
                    }
                    else {
                        ampm = 'PM';
                    }
                }
                
                $scope.rawvalue = $scope.rawvalue.substring( 0, $scope.rawvalue.lastIndexOf( ' ' )+1) + ampm;
                
            };
            
            $scope.preventBadValues = function( $event ){
                
                $scope.editing = true;
                
                var keyCode = $event.keyCode;
                
                if ( (keyCode < 35 || keyCode > 57) && keyCode !== 80 ){
                    $event.preventDefault();
                    return;
                }
            };
            
            $scope.keyUp = function( $event ){
                
                var input = getInput();
                $scope.pos = input.selectionStart;
                pos = $scope.pos;
                
                // backspace
                if ( $event.keyCode === 37 ){
                    var input = getInput();
                    var pos = input.selectionStart;
                    
                    if ( pos === 3 || pos === 6 || pos === 11 || pos === 14 || pos === 16 || pos === 17 ){
                        setSelection( pos - 2 );
                        $event.preventDefault();
                        return;
                    }
                }
                
                if ( $event.keyCode === 39 ){
                    
                    validateFields();
                    setSelection( pos + 1 );
                    $event.preventDefault();
                    return;
                }
                
                // month typed
                if ( pos == 2 ){
                    validateFields();
                    setSelection( pos + 1 );
                }
                // day
                else if ( pos == 5 ){
                    
                    validateFields();
                    setSelection( pos + 1 );
                }
                // year
                else if ( pos == 10 ){
                    validateFields();
                    setSelection( pos + 1 );
                }
                //hour
                else if ( pos == 13 ){
                    
                    validateFields();
                    setSelection( pos + 1 );
                }
                // minutes
                else if ( pos == 16 ){
                    validateFields();
                    setSelection( pos + 1 );
                }
                else if ( pos > 16 ){
                    validateFields();
                    setSelection( pos );
                }
            };
            
            $scope.apply = function(){
                $scope.editing = false;
                
                // set the real date object.
                var date = new Date();
                date.setFullYear( getValue( 6, 10 ));
                date.setMonth( getValue( 0, 2 ) - 1 );
                date.setDate( getValue( 3, 5 ) );
                date.setMinutes( getValue( 14, 16 ) );
                
                var h = getValue( 11, 13 );
                
                if ( $scope.mode == 12 ){
                    
                    var isPm = ( $scope.rawvalue.substr( 17 ) === 'PM' );
                    
                    if ( isPm === true && h !== 12 ){
                        h += 12;
                    }
                }

                date.setHours( h );

                if ( angular.isDate( $scope.date ) ){
                    $scope.date = date;
                }
                else {
                    $scope.date = date.getTime();
                }
            };
            
//            var init = function(){
//                setText();
//            };
//            
//            $timeout( init, 50 );
//            
            $scope.$watch( 'date', setText );
            
        },
        link: function( $scope, $element, $attr ){
            
            if ( !angular.isDefined( $attr.mode ) ){
                $attr.mode = 12;
            }
            
            if ( $attr.mode == 12 ){
                $scope.size = 19;
            }
            else {
                $scope.size = 16;
            }
        }
    };
    
});
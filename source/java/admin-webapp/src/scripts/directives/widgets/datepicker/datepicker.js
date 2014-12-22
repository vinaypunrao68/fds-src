angular.module( 'form-directives' ).directive( "datePicker", function(){
    
    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/widgets/datepicker/datepicker.html',
        scope: { dayLabels: '=?', validDates: '=?', date: '=ngModel', monthLabelFunction: '=?', selectedItems: '=?' },
        controller: function( $scope ){
            
            $scope.days = [];
            $scope.refDate;
            
            if ( !angular.isDefined( $scope.date ) ){
                $scope.date = new Date();
            }
            
            $scope.matchDay = function( day, otherDay ){
                
                if ( !angular.isDefined( otherDay ) ){
                    otherDay = new Date();
                }
                
                if ( day.getDate() === otherDay.getDate() &&
                    day.getMonth() === otherDay.getMonth() &&
                    day.getFullYear() === otherDay.getFullYear() ){
                    
                    return true;
                }
                
                return false;
            };  
            
            $scope.selectMe = function( day ){
                $scope.date = day.date;
                $scope.selectedItems = day.items;
            };
            
            $scope.monthLabel = function( month ){
                
                if ( angular.isFunction( $scope.monthLabelFunction ) ){
                    return $scope.monthLabelFunction( month );
                }
                
                switch( month ){
                    case 0:
                        return 'January';
                    case 1:
                        return 'February';
                    case 2:
                        return 'March';
                    case 3: 
                        return 'April';
                    case 4: 
                        return 'May';
                    case 5:
                        return 'June';
                    case 6: 
                        return 'July';
                    case 7:
                        return 'August';
                    case 8: 
                        return 'September';
                    case 9:
                        return 'October';
                    case 10:
                        return 'November';
                    case 11:
                        return 'December';
                    default:
                        return '';
                }
            };
            
            var buildCalendarDays = function(){
            
                if( !angular.isDefined( $scope.refDate )){
                    $scope.refDate = new Date();
                    $scope.refDate.setMonth( 1 );
                }
                
                $scope.days = [];
                
                var month = $scope.refDate.getMonth();
                var year = $scope.refDate.getFullYear();
                
                var refDate = new Date( $scope.refDate.getTime() );
                
                // find when the first is... our calendar starts at Sunday
                var startVal = 0-refDate.getDay();
                
                for ( var i = 0; i < 42; i++ ){
                    startVal++;
                    refDate = new Date();
                    refDate.setFullYear( year );
                    refDate.setMonth( month );
                    refDate.setDate( startVal );
                    
                    var enabled = true;
                    var items = [];
                    
                    if ( month !== refDate.getMonth() ){
                        enabled = false;
                    }
                    
                    var dayDate = new Date( refDate.getTime() );
                    
                    for ( var k = 0; angular.isDefined( $scope.validDates ) && k < $scope.validDates.length; k++ ){
                        var tDate = new Date( $scope.validDates[k] );
                        
                        if ( $scope.matchDay( dayDate, tDate ) === true ){
                            items.push( $scope.validDates[k] );
                        }
                    }
                    
                    $scope.days[i] = { date: dayDate, enabled: enabled, items: items };
                }
            };  
            
            $scope.backMonth = function(){    
                $scope.refDate.setMonth( $scope.refDate.getMonth() - 1 );
                buildCalendarDays();
            };
            
            $scope.aheadMonth = function(){
                $scope.refDate.setMonth( $scope.refDate.getMonth() + 1 );
                buildCalendarDays();
            };
            
            $scope.backYear = function(){
                $scope.refDate.setFullYear( $scope.refDate.getFullYear() - 1 );
                buildCalendarDays();
            };
            
            $scope.aheadYear = function(){
                $scope.refDate.setFullYear( $scope.refDate.getFullYear() + 1 );
                buildCalendarDays();
            };
            
            buildCalendarDays();
        }
    };
    
});
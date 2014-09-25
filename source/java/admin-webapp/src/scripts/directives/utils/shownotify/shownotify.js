angular.module( 'utility-directives' ).directive( 'showNotify', function(){

    return {
        restrict: 'A',
        controller: function( $scope, $element ){

            var determineVisibility = function( str ){

                if ( str.indexOf( 'ng-show' ) !== -1 ){
                    return true;
                }
                else if ( str.indexOf( 'ng-hide' ) !== -1 ){
                    return false;
                }
                else {
                    return true;
                }
            };

            $scope.$watch( function(){ return $element[0].getAttribute( 'class' ); }, function( newList, oldList ){

                var classes = $element[0].getAttribute( 'class' );

                var oldShown = determineVisibility( oldList );
                var newShown = determineVisibility( newList );

                if ( oldShown === true && newShown === false ){
                    $scope.$emit( 'fds::page_hidden' );
                }
                else if ( oldShown === false && newShown === true ){
                    $scope.$emit( 'fds::page_shown' );
                }

            });
        }
    };

});

angular.module( 'form-directives' ).directive( 'prioritySlider', function(){
   return {
       restrict: 'E',
       replace: true,
       transclude: false,
       scope: { priority: '=' },
       templateUrl: 'scripts/directives/widgets/priorityslider/priorityslider.html',
       controller: function( $scope ){
           $scope.priorities = [10,9,8,7,6,5,4,3,2,1];
           $scope.widthPer = '9%';

           if ( !angular.isDefined ( $scope.priority ) ){
               $scope.priority = 10;
           }

           $scope.setPriority = function( p ){
               $scope.priority = p;
           }
       }
   };
});

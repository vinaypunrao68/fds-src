angular.module( 'form-directives' ).directive( 'linkbuttons', function(){

    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        // item format is:
        // [{label: 'name', callback: function(){}, notifications: #, selected: false/true},...]
        scope: { items: '=' },
        templateUrl: 'scripts/directives/widgets/linkbuttons/linkbuttons.html',
        controller: function(){


        }
    };
});

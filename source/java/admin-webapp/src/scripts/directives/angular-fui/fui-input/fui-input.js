angular.module( 'angular-fui' ).directive( 'fuiInput', function(){

    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/angular-fui/fui-input/fui-input.html',
        scope: { aliasedModel : '=ngModel', placeholder: '@', iconClass: '@', type: '@', disabled: '=?', noBorder: '@' },
        controller: function( $scope ){
        },
        link: function( $scope, $element, $attrs ){

            if ( angular.isDefined( $attrs.type ) ){

                if ( $attrs.type !== 'password' && $attrs.type !== 'text' ){
                    $attrs.type = 'text';
                }
            }
            else{
                $attrs.type = 'text';
            }
            
            if ( !angular.isDefined( $attrs.disabled ) ){
                $attrs.disabled = false;
            }
        }
    };
});

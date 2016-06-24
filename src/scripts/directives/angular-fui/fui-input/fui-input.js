angular.module( 'angular-fui' ).directive( 'fuiInput', function(){

    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/angular-fui/fui-input/fui-input.html',
        // For ragular expression validity checks... this widget has it's own ng-form so to access the validity of this input
        // you'll need to access it as {{name}}.{{name}}.$valid rather than with any form name you might have above.
        scope: { aliasedModel : '=ngModel', placeholder: '@', iconClass: '@', type: '@', disabled: '=?', noBorder: '@', name: '@', regexp: '@', errorText: '@', required: '=?' },
        controller: function( $scope, $element ){
            
            $scope.show_red = false;
            $scope.errorColor = 'rgba( 255, 0, 0, 0.9 )';
            $scope.top = '47px';
            $scope.left = '0px';
            $scope.below = true;
            $scope.leftSide = true;
            
            if ( !angular.isDefined( $scope.required ) ){
                $scope.required = false;
            }
            
            var checkRegexp = function(){
                
                var valid = true;
                
                if ( angular.isDefined( $scope.regexp ) ){
                    
                    valid = $scope.regexp.test( $scope.aliasedModel );
                }
                
                return valid;
            };
            
            $scope.validate = function( $event ){
                
                // bail right away if there are no validation requirements
                if ( $scope.required === false &&
                    !angular.isDefined( $scope.regexp ) ){
                    return;
                }
                
                var valid = true;
                
                if ( !angular.isDefined( $scope.aliasedModel ) ){
                    $scope.aliasedModel = '';
                }
                 
                if ( $scope.required === true ){

                    if ( $scope.aliasedModel.length === 0 ){
                        valid = false;
                    }
                    else {
                        valid = checkRegexp();
                    }
                }
                else {

                    if ( $scope.aliasedModel.length !== 0 ){
                        valid = checkRegexp();
                    }
                }
                
                var formItem = $scope[$scope.name];
                var inputItem = formItem[$scope.name];

                inputItem.$setValidity( "regexp", valid );
                $scope.show_red = !valid;
            };
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
            
            if ( !angular.isDefined( $attrs.name ) ){
                $attrs.name = '' + (new Date()).getTime() + Math.random() * Number.MAX_VALUE;
            }
            
            if ( angular.isDefined( $attrs.regexp ) && !$attrs.regexp.hasOwnProperty( 'test' ) ){
                $attrs.regexp = new RegExp( $attrs.regexp );
            }
            
            if ( !angular.isDefined( $attrs.errorText ) ){
                $attrs.errorText = "Invalid data";
            }
        }
    };
});

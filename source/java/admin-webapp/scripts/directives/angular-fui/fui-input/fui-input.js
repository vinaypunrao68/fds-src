angular.module("angular-fui").directive("fuiInput",function(){return{restrict:"E",replace:!0,transclude:!1,templateUrl:"scripts/directives/angular-fui/fui-input/fui-input.html",scope:{aliasedModel:"=ngModel",placeholder:"@",iconClass:"@",type:"@"},controller:function(){},link:function($scope,$element,$attrs){angular.isDefined($attrs.type)?"password"!==$attrs.type&&"text"!==$attrs.type&&($attrs.type="text"):$attrs.type="text"}}});
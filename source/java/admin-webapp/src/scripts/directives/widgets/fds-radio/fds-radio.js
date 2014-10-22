angular.module('form-directives').directive('fdsRadio', function () {

  return {
    restrict: 'E',
    replace: true,
    transclude: false,
    templateUrl: 'scripts/directives/widgets/fds-radio/fds-radio.html',
    scope: { selectedValue: '=ngModel', name: '@', value: '@', label: '@', enabled: '=?' },
    controller: function ($scope) {

      var localEnabled = true;

      if (!angular.isDefined($scope.enabled)) {
        $scope.enabled = localEnabled;
      }

      $scope.buttonClicked = function () {

        if ($scope.enabled === true) {

          $scope.selectedValue = $scope.value;
        }
      };
    },
    link: function ($scope, $element, $attrs) {
    }
  };
});
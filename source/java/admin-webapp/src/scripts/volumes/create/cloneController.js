angular.module('volumes').controller('cloneVolumeController', ['$scope', '$volume_api', '$filter', function ($scope, $volume_api, $filter) {

  var init = function () {
    $scope.volumeVars.toClone = 'new';

    if (!angular.isDefined($scope.selectedItem) || !angular.isDefined($scope.selectedItem.name)) {
      $scope.selectedItem = {name: 'None'};
    }

    $scope.choosing = false;
  };

  init();

  $scope.cloneOptions = [];
  var currentStateLabel = $filter('translate')('volumes.l_current_state');

  $scope.cloneColumns = [
    { title: 'Volumes', property: 'name' },
    { title: 'Snapshots', property: 'name' }
  ];

  $scope.getSelectedName = function () {

    if ($scope.selectedItem.name === currentStateLabel) {
      return $scope.selectedItem.parent.name;
    }

    return $scope.selectedItem.name;
  };

  $scope.getSnapshots = function (volume, callback) {
    $volume_api.getSnapshots(volume.id, function (results) {
      var current = { data_connector: volume.data_connector,
        sla: volume.sla, limit: volume.limit, priority: volume.priority,
        id: volume.id, name: currentStateLabel };

      results.splice(0, 0, current);

      callback(results);
    });
  };

  $scope.cancel = function () {
    $scope.choosing = false;
  };

  $scope.choose = function () {
    $scope.choosing = false;
//        $scope.volumeVars.clone = $scope.selectedItem;

    // if it's a snapshot we need to fill in the parent volumes
    // settings
    if (angular.isDefined($scope.selectedItem.parent)) {
      $scope.volumeVars.clone = $scope.selectedItem.parent;
      $scope.volumeVars.clone.id = $scope.selectedItem.id;
      $scope.volumeVars.clone.name = $scope.selectedItem.name;
      $scope.volumeVars.clone.cloneType = 'snapshot';
    }
    else {
      $scope.volumeVars.clone = $scope.selectedItem;
    }
  };

  $scope.$watch('selectedItem', function (newVal) {
  });

  var refresh = function (newValue) {

    if (newValue === false) {
      return;
    }

    $scope.cloneOptions = $volume_api.volumes;
    $scope.selectedItem = $scope.cloneOptions[0];

    init();
  };

  $scope.$watch('volumeVars.creating', refresh);
  $scope.$watch('volumeVars.editing', refresh);

}]);
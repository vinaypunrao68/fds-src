angular.module( 'volumes' ).controller( 'cloneVolumeController', ['$scope',  function( $scope ){
    
    $scope.volumeVars.toClone = 'new';
    $scope.selectedItem = {name: 'None'};
    $scope.choosing = false;
    
    $scope.cloneOptions = [
        { name: 'Volume 1', children: [
            {name: 'SNAP1'}
        ]},
        { name: 'Volume 2', children: [
            {name: 'SNAP1'}
        ]},
        { name: 'Volume 3', children: [
            {name: 'SNAP1'}
        ]}        
    ];
    
    $scope.cloneColumns = [
        { name: 'Name', property: 'name' }
    ];
    
    $scope.choose = function(){
        $scope.choosing = false;
    };
    
    $scope.$watch( 'selectedItem', function( newVal ){
        
        var i = 0;
    });
    
}]);
angular.module( 'inbox' ).controller( 'inboxController', ['$scope', '$inbox_service', '$authorization', function( $scope, $inbox_service, $authorization ){

    $scope.items = [{
            label: 'All',
            notifications: 0,
            selected: true
        },
        {
            label: 'Warnings',
            notifications: 0
        },
        {
            label: 'Alerts',
            notifications: 1
        },
        {
            label: 'Actions',
            notifications: 1
        },
        {
            label: 'FYI',
            notifications: 0
        },
        {
            label: 'Recommendations',
            notifications: 3
        }];

    $scope.messages = [];

    $scope.messagesReceived = function( response ){
        $scope.messages = eval( response );
    };

    $scope.isAllowed = function( permission ){
        return $authorization.isAllowed( permission );
    };

    $inbox_service.getMessages( $scope.messagesReceived );
}]);

angular.module( 'main' ).controller( 'mainController', ['$scope', '$authentication', '$authorization', '$state', '$domain_service', function( $scope, $authentication, $authorization, $state, $domain_service ){

    $scope.priority = 10;
    $scope.rememberChecked = false;
    $scope.selected = {};
    $scope.domains = $domain_service.getDomains();
//    $scope.loggedInUser = $authorization.getUsername();

    $scope.menuItems = [{name: 'Account Details'},{name: 'Change Password'},{name: '-'},{ name: 'Logout'}];

    $scope.views = [
        { id: 'status', link: 'homepage.status', text: 'Status', iconClass: 'icon-system', selected: false },
        { id: 'inbox', link: 'homepage.inbox', text: 'Inbox', iconClass: 'icon-inbox', selected: false },
        { id: 'activity', link: 'homepage.activity', text: 'Activity', iconClass: 'icon-activity_pulse', selected: false },
        { id: 'system', link: 'homepage.system', text: 'System', iconClass: 'icon-icon_8948', selected: false, permission: 'System Management' },
        { id: 'volumes', link: 'homepage.volumes', text: 'Volumes', iconClass: 'icon-volumes', selected: false },
        { id: 'users', link: 'homepage.users', text: 'Users', iconClass: 'icon-users', selected: false },
        { id: 'admin', link: 'homepage.admin', text: 'Admin', iconClass: 'icon-icon_8948', selected: false }
    ];

    $scope.navigate = function( item ) {

        $scope.views.forEach ( function( view ){
            view.selected = false;
        });

        item.selected = true;

        $state.transitionTo( item.link );
    };

    $scope.login = function(){
        $authentication.login( $scope.username, $scope.password );
    };

    $scope.logout = $authentication.logout;

    $scope.isAllowed = function( permission ){

        if ( !angular.isDefined( permission ) ){
            return true;
        }

        return $authorization.isAllowed( permission );
    };

    $scope.keyEntered = function( $event ){
        if ( $event.keyCode == 13 ){
            $scope.login();
        }
    };

    $scope.itemSelected = function( item ){

        if ( item === $scope.menuItems[3] ){
            $scope.logout();
        }
        else if ( item === $scope.menuItems[0] || item === $scope.menuItems[1] ){
            var o = {
                link: 'homepage.account'
            };

            $scope.navigate( o );
        }
    };

    $scope.$watch( 'loggedInUser', function( newValue ){

        $scope.itemSelected( newValue );
    });

    $scope.$watch( function(){ return $authentication.isAuthenticated; }, function( val ) {
        $scope.loggedInUser = $authorization.getUsername();
        $scope.validAuth = val;
        $scope.username = '';
        $scope.password = '';
    });

    $scope.$watch( function(){ return $authentication.error; }, function( val ) {
        $scope.error = val;
    });

    // one-time init stuff

    // determine which view is selected
    var noState = true;
    $scope.views.forEach( function( view ){
        if ( $state.current.name === view.link ){
            view.selected = true;
            noState = false;
        }
    });

    if ( noState ){
        // if we get here, nothing was selected
        $scope.navigate( $scope.views[0] );
    }

}]);

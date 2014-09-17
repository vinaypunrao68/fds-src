app = angular.module( 'formation', ['ui.router', 'main', 'volumes', 'system', 'inbox', 'status', 'activity-management', 'user-page', 'admin-settings'] );

app.config( function( $stateProvider, $urlRouterProvider ){

    $urlRouterProvider.otherwise( '/home' );

    $stateProvider
        .state( 'homepage', {
            url: '/home',
            templateUrl: 'scripts/main/main.html'
        })
        .state( 'homepage.volumes', {
            url: '/volumes',
            templateUrl: 'scripts/volumes/volumes.html'
        })
        .state( 'homepage.system', {
            url: '/system',
            templateUrl: 'scripts/system/system.html'
        })
        .state( 'homepage.status', {
            url: '/status',
            templateUrl: 'scripts/status/status.html'
        })
        .state( 'homepage.users', {
            url: '/users',
            templateUrl: 'scripts/users/users.html'
        })
        .state( 'homepage.inbox', {
            url: '/inbox',
            templateUrl: 'scripts/inbox/inbox.html'
        })
        .state( 'homepage.activity', {
            url: '/activity',
            templateUrl: 'scripts/activity/activity.html'
        })
        .state( 'homepage.admin', {
            url: '/admin',
            templateUrl: 'scripts/admin/admin.html'
        })
        .state( 'homepage.account', {
            url: '/accountdetails',
            templateUrl: 'scripts/account/account.html'
        });
});

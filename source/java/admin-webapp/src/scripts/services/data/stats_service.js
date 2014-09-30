angular.module( 'statistics' ).factory( '$stats_service', ['$http', function( $http ){

    var service = {};

    service.getFirebreakSummary = function(){

        return {
            vals: [
                { volume: 'Vol 1', size: 100, minSinceLastFirebreak: 1440 },
                { volume: 'Vol 2', size: 45, minSinceLastFirebreak: 10 },
                { volume: 'Vol 3', size: 8, minSinceLastFirebreak: 3456 },
                { volume: 'Vol 4', size: 85, minSinceLastFirebreak: 4400 },
                { volume: 'Vol 5', size: 16, minSinceLastFirebreak: 1000 },
                { volume: 'Vol 6', size: 20, minSinceLastFirebreak: 1266 },
                { volume: 'Vol 7', size: 37, minSinceLastFirebreak: 2 },
                { volume: 'Vol 8', size: 21, minSinceLastFirebreak: 1700 },
                { volume: 'Vol 9', size: 94, minSinceLastFirebreak: 912 },
                { volume: 'Vol 10', size: 11, minSinceLastFirebreak: 1300 },
                { volume: 'Vol 11', size: 61, minSinceLastFirebreak: 1200 }
            ]
        };

    };

    return service;
}]);

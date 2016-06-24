angular.module( 'form-directives' ).directive( 'pager', function(){
    
    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/widgets/pager/pager.html',
        scope: { nextLabel: '@', backLabel: '@', data: '=ngModel', filter: '=', call: '=' },
        controller: function( $scope ){
            
            $scope.pages = [];
            $scope.currentPage = 6;
            $scope.totalPages = 0;
            
            // page object should look like this:
            //
            // { pageStart: index, pageEnd: index, totalRecords: num, pageSize: size, records: [] }
            var dataReturned = function( data ){
                
                $scope.totalPages = data.totalRecords / data.pageSize;
                $scope.pages = [];
                
                var startPage = $scope.currentPage - 4;
                var endPage = $scope.currentPage + 5;
            
                while( startPage < 0 ){
                    startPage++;
                    endPage++;
                }
                
                if ( endPage > $scope.totalPages ){
                    endPage = $scope.totalPages - 1;
                    startPage -= (10 - (endPage - startPage) );
                }
                
                for ( var i = startPage; i < endPage; i++ ){
                    $scope.pages.push( {page: i, start: i*data.pageSize } );
                }
                
                $scope.data = data.records;
            };
            
            $scope.goto = function( page ){
                
                if ( $scope.pages.length > page ){
                    $scope.filter.startIndex = $scope.pages[page].start;
                }
                else {
                    $scope.filter.startIndex = 0;
                }
                
                $scope.currentPage = page;
                $scope.call( $scope.filter, dataReturned );
            };
            
            $scope.goto( $scope.currentPage );
        }
    };
    
});

angular.module('charts').directive('treemap', function () {

  return {
    restrict: 'E',
    replace: true,
    transclude: false,
    templateUrl: 'scripts/directives/charts/treemap/treemap.html',
    // the tooltip element here is a call back to let the implementor
    // set the text.
    // colorProerty is the property this class will use to pick a color for the box
    scope: { data: '=', tooltip: '=', colorProperty: '@' },
    controller: function ($scope, $element, $resize_service) {

      var root = {};

      var $colorScale;
      $scope.hoverEvent = {};
      $scope.tooltipText = 'None';

      var buildColorScale = function () {

        var max = d3.max($scope.data.series, function (d) {

          if (!angular.isDefined(d) || !angular.isDefined(d.datapoints)) {
            return 0;
          }

          if (d.datapoints[0].hasOwnProperty($scope.colorProperty)) {
            return d.datapoints[0][$scope.colorProperty];
          }
          return d.datapoints[0].y;
        });

        $colorScale = d3.scale.linear()
            .domain([max, 3600 * 12, 3600 * 6, 3600 * 3, 3600, 0])
            .range(['#389604', '#68C000', '#C0DF00', '#FCE300', '#FD8D00', '#FF5D00'])
            .clamp(true);
      };

      var computeMap = function (c) {

        if (!angular.isDefined($scope.data.series)) {
          return;
        }

        // preserve the y value
        for (var i = 0; i < $scope.data.series.length; i++) {

          if (!angular.isDefined($scope.data.series[i].datapoints)) {
            continue;
          }

//                    if ( !angular.isDefined( $scope.data[i].secondsSinceLastFirebreak ) ){
          if (!$scope.data.series[i].datapoints[0].hasOwnProperty($scope.colorProperty)) {
            $scope.data.series[i][$scope.colorProperty] = $scope.data.series[i].datapoints[0].y;
          }
        }

        var map = d3.layout.treemap();
        var nodes = map
            .size([$element.width(), $element.height()])
            .sticky(true)
            .children(function (d) {

              return d;
            })
            .value(function (d, i, j) {

              if (!angular.isDefined(d.datapoints)) {
                return 0;
              }

              if (angular.isDefined(d.datapoints[0].value)) {
                return d.datapoints[0].value;
              }

              return d.datapoints[0].x;
            })
            .nodes($scope.data.series);
      };

      var create = function () {

        buildColorScale();
        computeMap();

        root = d3.select('.chart');

        root.selectAll('.node').remove();

        root.selectAll('.node').data($scope.data.series).enter()
            .append('div')
            .classed({'node': true })
            .attr('id', function (d, i) {
              return 'volume_' + i;
            })
            .style({'position': 'absolute'})
            .style({'width': function (d) {
              return d.dx;
            }})
            .style({'height': function (d) {
              return d.dy;
            }})
            .style({'left': function (d) {
              return d.x;
            }})
            .style({'top': function (d) {
              return d.y;
            }})
            .style({'border': '1px solid white'})
            .style({'background-color': function (d) {
              if (d.hasOwnProperty($scope.colorProperty)) {
                return $colorScale(d[ $scope.colorProperty ]);
              }
              return $colorScale(0);
            }})
            .on('mouseover', function (d) {
              if (angular.isFunction($scope.tooltip)) {
                $scope.tooltipText = $scope.tooltip(d);
                var el = $($element[0]).find('.tooltip-placeholder');
                el.empty();
                var ttEl = $('<div/>').html($scope.tooltipText).contents();
                el.append(ttEl);
              }
            });
      };

      $scope.$on('$destory', function () {
        $resize_service.unregister($scope.id);
      });

      $scope.$watch('data', create);

      create();

      $resize_service.register($scope.id, create);
    }
  };

});

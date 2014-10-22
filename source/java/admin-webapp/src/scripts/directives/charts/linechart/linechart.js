angular.module('charts').directive('lineChart', function () {

  return {
    restrict: 'E',
    replace: true,
    transclude: false,
    templateUrl: 'scripts/directives/charts/linechart/linechart.html',
    scope: { data: '=', colors: '=?', opacities: '=?', drawPoints: '@', yAxisLabelFunction: '=', axisColor: '@',
      tooltip: '=?', lineColors: '=?', lineStipples: '=?' },
    controller: function ($scope, $element, $resize_service) {

      $scope.hoverEvent = false;

      var root = {};

      var $left_label = 0;
      var $left_padding = 0;
      var $right_padding = 0;
      var $bottom_labels = 0;
      var $top_padding = 0;
      var $bottom_padding = 0;
      var chartData = [];
      var $xScale;
      var $yScale;
      var $max;
      var $svg;

      if ($scope.drawPoints !== true && $scope.drawPoints !== false) {
        $scope.drawPoints = false;
      }

      if (!angular.isDefined($scope.axisColor)) {
        $scope.axisColor = 'black';
      }

      var el = d3.select($element[0]).select('.line-chart');

      $svg = el.append('svg')
          .attr('width', '100%')
          .attr('height', '100%');

      // calculate this off of real data.
      var buildMax = function () {
        $max = d3.max($scope.data.series, function (d) {
          return d3.max(d.datapoints, function (s) {
            return s.y;
          });
        });
      };

      // calculate off of real data.  Depends on max calculation
      var buildScales = function () {

        buildMax();

        var xMax = 1;

        if (angular.isDefined($scope.data.series[0]) && angular.isDefined($scope.data.series[0].datapoints)) {
          xMax = $scope.data.series[0].datapoints.length - 1;
        }

        $xScale = d3.scale.linear()
          // all series must have the same number of points
            .domain([0, xMax])
            .range([$left_label + $left_padding, $element.width() - $right_padding]);

        $yScale = d3.scale.linear()
            .domain([ 0, $max ])
            .range([$element.height() - $top_padding, $bottom_labels + $bottom_padding]);
      };

      var clean = function () {
        $svg.selectAll('.series-group').remove();
      };

      var removeGuides = function () {

        $svg.selectAll('.guide-lines').remove();
      };

      var update = function () {

        removeGuides();
        buildScales();

        var area = d3.svg.area()
            .x(function (d) {
              return $xScale(d.x);
            })
            .y0(function (d) {
              return $yScale(0);
            })
            .y1(function (d) {
              return $yScale(d.y);
            })
            .interpolate('monotone');

        $svg.selectAll('.line')
            .transition()
            .duration(500)
            .attr('d', function (d) {
              return area(d.datapoints);
            });

        $svg.selectAll('.point')
            .transition()
            .duration(500)
            .attr('cy', function (d) {
              return $yScale(d.y);
            });

        var chunks = Math.round($max / 4);

        chunks = [chunks, chunks * 2, chunks * 3 ];

        var guideGroup = $svg.selectAll('.guide-lines').data(chunks).enter()
            .append('g')
            .attr('class', 'guide-lines');

        guideGroup.selectAll('.guide-lines').data(chunks).enter()
            .append('line')
            .attr('x1', $xScale(0))
            .attr('y1', function (d) {
              return $yScale(d);
            })
            .attr('x2', function (d) {

              var val = 0;

              if (angular.isDefined($scope.data.series[0]) && angular.isDefined($scope.data.series[0].datapoints)) {
                val = $xScale($scope.data.series[0].datapoints.length - 1);
              }
              else {
                val = $xScale(0);
              }

              return val;
            })
            .attr('y2', function (d) {
              return $yScale(d);
            })
            .attr('stroke', $scope.axisColor)
            .attr('stroke-dasharray', '3,3');

        guideGroup.selectAll('.guide-lines').data(chunks).enter()
            .append('text')
            .text(function (d) {

              if (angular.isFunction($scope.yAxisLabelFunction)) {
                return ( $scope.yAxisLabelFunction(d) );
              }

              return d;
            })
            .attr('x', $xScale(0))
            .attr('y', function (d) {
              return $yScale(d) - 4;
            })
            .attr('fill', $scope.axisColor)
            .attr('font-size', '11px');

        // baseline
        $svg.append('line')
            .attr('class', 'guide-lines')
            .attr('x1', $xScale(0))
            .attr('x2', function (d) {

              if (angular.isDefined($scope.data.series[0]) && angular.isDefined($scope.data.series[0].datapoints)) {
                return $xScale($scope.data.series[0].datapoints.length - 1);
              }

              return $xScale(0);
            })
            .attr('y1', $yScale(0))
            .attr('y2', $yScale(0))
            .attr('stroke', 'black');

      };

      var create = function () {

        clean();
        buildScales();

        var area = d3.svg.area()
            .x(function (d) {
              return $xScale(d.x);
            })
            .y0(function (d) {
              return $yScale(0);
            })
            .y1(function (d) {
              return $yScale(0);
            })
            .interpolate('monotone');

        var groups = $svg.selectAll('.series-group').data($scope.data.series).enter()
            .insert('g', ':first-child')
            .attr('class', 'series-group');

        groups.append('path')
            .attr('stroke', function (d, i) {
              if ($scope.lineColors && $scope.colors.length > i) {
                return $scope.lineColors[i];
              }

              return '#ACACAC';
            })
            .attr('stroke-dasharray', function (d, i) {

              if ($scope.lineStipples && $scope.lineStipples.length > i) {
                return $scope.lineStipples[i];
              }

              return null;
            })
            .attr('fill', function (d, i) {
              if ($scope.colors && $scope.colors.length > i) {
                return $scope.colors[i];
              }

              return 'black';
            })
            .attr('fill-opacity', function (d, i) {
              if ($scope.opacities && $scope.opacities.length > i) {
                return $scope.opacities[i];
              }

              return 0.8;
            })
            .attr('class', 'line')
            .attr('d', function (d) {
              return area(d.datapoints);
            })
            .on('mouseover', function (d, i, j) {
              if (angular.isFunction($scope.tooltip)) {
                $scope.tooltipText = $scope.tooltip(d, i, j);
                var el = $($element[0]).find('.tooltip-placeholder');
                el.empty();
                var ttEl = $('<div/>').html($scope.tooltipText).contents();
                el.append(ttEl);
              }
            });


        groups.selectAll('series-group')
            .data(function (d, i) {
              return d;
            }).enter()
            .append('circle')
            .attr('class', 'point')
            .attr('cx', function (d) {
              return $xScale(d.x);
            })
            .attr('cy', function (d) {
              return 0;
            })
            .attr('r', 3)
            .attr('fill', 'white')
            .attr('stroke', 'black')
            .attr('style', function (d) {
              if ($scope.drawPoints === true) {
                return 'visiblity: visible';
              }

              return 'visibility: hidden';
            });

        update();
      };

      $scope.setHoverEvent = function ($event) {

        if (angular.isFunction($scope.tooltip)) {
          $event.target = $event.currentTarget;
          $scope.hoverEvent = $event;
        }
      };

      create();

      $resize_service.register($scope.$id, update);

      $scope.$watch('data', function (oldVal, newVal) {

      });
    }
  };

});

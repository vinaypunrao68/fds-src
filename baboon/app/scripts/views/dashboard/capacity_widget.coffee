define (require, exports, modules) ->
  Backbone = require('backbone')
  templates = require('templates')

  class CapacityWidget extends Backbone.View
    template: templates['app/scripts/templates/dashboard/capacity_widget.jst']
    className: 'block_4'

    initialize: (opts) ->
      @volumes = opts.volumes

    render: =>
      @$el.html(@template(volumes: @volumes))
      graph = new CapacityChart
        node: @$('svg.chart.capacity')[0]
        volumes: @volumes
      graph.generate()

      pie = new CurrentConsumptionChart
        node: @$('svg.chart.consumption')[0]
        volumes: @volumes
      pie.generate()

      this

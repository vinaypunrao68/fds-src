define (require, exports, modules) ->
  Utils = require('utils')
  nv = require('nvd3')

  class CurrentConsumptionChart

    constructor: (opts) ->
      @volumes = opts.volumes
      @node = opts.node

    generate: ->
      chart_data = @getChartData()
      window.ww = chart_data
      click_event = @click_event
      node = @node
      margin = {top:0, right:0, bottom:0, left:0}
      @chart = nv.models.pieChartFormation()
            .x( (d) -> d.label )
            .y( (d) -> d.value )
            .color( (d) -> d.data.color )
            .showLabels(false)
            .showLegend(false)
            .margin(margin)
            .tooltips(false)
            .donut(true)
            .pieStartAngle(3.66)
            .pieEndAngle(8.9)
            .centerPercent('89')
            .bottomLabel('Current Consumption')
      chart = @chart

      nv.addGraph(
        generate: ->
          d3.select(node)
            .datum(chart_data)
            .transition().duration(500)
            .call(chart)
          chart
      )

    getChartData: ->
      levels = {}
      @volumes.each (volume) ->
        levels[volume.get('limiting')] ?= 0
        levels[volume.get('limiting')] += _.last(volume.get('capacity'))[1]

      data = []
      # for key, val of levels
      #   data.push({label: key, value: val, color: Formation.Util.limit_colors[key] })
      # Data in order
      for idx in [0..2]
        data.push({label: idx, value: levels[idx], color: Utils.limit_colors[idx] })
      data

define (require, exports, modules) ->
  Utils = require('utils')
  nv = require('nvd3')

  class CapacityChart

    constructor: (opts) ->
      @volumes = opts.volumes
      @node = opts.node

    generate: ->
      chart_data = @getChartData()
      click_event = @click_event
      node = @node
      margin = {top:20, right:20, bottom:20, left:20}
      @chart = nv.models.stackedAreaChartFormation()
            .x( (d) -> Date.parse(d[0]) )
            .y( (d) -> d[1] )
            .forceY([0,100])
            .showControls(false)
            .showLegend(false)
            .enableToggle(false)
            .margin(margin)
            .transitionDuration(5550)
      @chart.xAxis
        .tickFormat( (d) -> d3.time.format('%m-%d')(new Date(d) ) )
      chart = @chart

      nv.addGraph(
        generate: =>
          d3.select(node)
            .datum(chart_data)
            .transition().duration(500)
            .call(chart)

          nv.utils.windowResize(chart.update);
          chart
      )

    getChartData: ->
      @volumes.map(@chartDataItem)

    chartDataItem: (volume) ->
      {key: volume.get('name'), values: volume.get('capacity'), color: Utils.limit_colors[volume.get('limiting') || 0]}


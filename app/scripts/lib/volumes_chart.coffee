define (require, exports, modules) ->
  Backbone = require('backbone')
  Utils = require('utils')
  nv = require('nvd3')

  class VolumesChart

    constructor: (opts) ->
      @single = opts.single
      @click_event = opts.click_event
      @volumes = opts.volumes
      @node = opts.node
      @volumes.on('updated', @update) unless @single
      @volumes.on('change:priority', @update) unless @single

    generate: ->
      chart_data = @getChartData()
      click_event = @click_event
      node = @node
      margin = if @single then {top:0, right:0, bottom:0, left:0} else {top:20, right:20, bottom:20, left:10}
      @chart = nv.models.multiBarChartFormation()
            .x( (d) -> d.label )
            .y( (d) -> d.value )
            .lineY( (d) -> d.sla )
            .limitY( (d) -> d.limit )
            .forceY([0,100])
            .tooltip( (key, x, y, e, graph) ->
              "<h3>#{x}</h3><p>#{y}</p>"
            )
            .showControls(false)
            .showLegend(false)
            .margin(margin)
            .groupSpacing(0.1)
            .delay(100)
            .transitionDuration(5550)
      chart = @chart
      nv.addGraph(
        generate: =>
          d3.select(node)
            .datum([{key: 'TITLE', values: chart_data }])
            .call(chart)

          nv.utils.windowResize(chart.update);
          chart

        callback: (chart) ->
          if click_event == 'element'
            chart.multibar.dispatch.on 'elementClick', (obj) ->
              data = obj.series.values[obj.pointIndex]
              Backbone.history.navigate("volume/#{data.id}", true)
      )

    update: =>
      d3.select(@node)
        .datum([{key: 'TITLE', values: @getChartData()}])
        .transition().duration(500)
        .call(@chart)

    getChartData: ->
      if @single then [ @chartDataItem(@volumes[0]) ] else @volumes.map(@chartDataItem)
    chartDataItem: (volume) ->
      {id: volume.get('id'), label: volume.get('name'), value: volume.get('performance'), limit: volume.get('limit'), sla: volume.get('sla'), color: Utils.limit_colors[volume.get('limiting') || 0]}

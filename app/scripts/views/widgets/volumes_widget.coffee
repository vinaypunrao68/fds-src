define (require, exports, modules) ->
  Backbone = require('backbone')
  templates = require('templates')
  VolumesChart = require('lib/volumes_chart')

  class VolumesWidget extends Backbone.View
    template: templates['app/scripts/templates/dashboard/volumes_widget.jst']
    className: 'block_4'

    events:
      'click': 'show_volumes'
      'dblclick': 'stopAnimation'

    initialize: (opts) ->
      @volumes = opts.volumes
      @title = opts.title
      @click_event = opts.click || 'chart'

    render: =>
      @$el.html(@template(title: @title, click_event: @click_event))
      graph = new VolumesChart
        single: false
        node: @$('svg.chart')[0]
        click_event: @click_event
        volumes: @volumes
      graph.generate()
      this

    stopAnimation: ->
      @volumes.disableLiveUpdate() # for development

    show_volumes: ->
      if @click_event == 'chart'
        Backbone.history.navigate("volumes", true)

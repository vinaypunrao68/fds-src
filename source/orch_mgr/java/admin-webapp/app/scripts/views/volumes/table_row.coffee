define (require, exports, modules) ->
  _ = require('underscore')
  Backbone = require('backbone')
  templates = require('templates')
  VolumesForm = require('views/volumes/form')
  nv = require('nvd3')

  class VolumesTableRow extends Backbone.View
    template: templates['app/scripts/templates/volumes/table_row.jst']

    tagName: 'tr'

    events:
      'click td': 'show_volume'

    initialize: ->
      @undelegateEvents()
      @delegateEvents()

    render: ->
      @$el.html(@template(volume: @model))

      node = @$('svg')[0]
      data = ({x: item[0], y: item[1]} for item in @model.get('usage_history'))
      nv.addGraph ->
        chart = nv.models.sparkline()
        chart
          .width(50)
          .color( -> '#0080FD')
          .x( (d, i) -> i )
        d3.select(node).datum(data).transition().duration(250).call(chart)
        chart
      this

    show_volume: (e) ->
      e.preventDefault()
      volume_view = new VolumesForm(model: @model)
      volume_view.popup()

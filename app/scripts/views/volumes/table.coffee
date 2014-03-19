define (require, exports, modules) ->
  _ = require('underscore')
  Backbone = require('backbone')
  templates = require('templates')
  VolumeTableRow = require('views/volumes/table_row')

  class VolumesTable extends Backbone.View
    template: templates['app/scripts/templates/volumes/table.jst']

    initialize: (opts) ->
      @volumes = opts.volumes
      @volumes.on('change:priority', @render)

    render: =>
      @$el.html(@template())
      table = @$('tbody')
      @volumes.sort() # Calls reset
      @volumes.each (volume) ->
        view = new VolumeTableRow(model: volume)
        table.append(view.render().el)
      this

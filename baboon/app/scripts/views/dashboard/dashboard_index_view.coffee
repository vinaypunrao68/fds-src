define (require, exports, modules) ->
  Backbone = require('backbone')
  templates = require('templates')
  MiniWidget = require('views/dashboard/mini_widget')
  CapacityWidget = require('views/dashboard/capacity_widget')
  StorageWidget = require('views/dashboard/storage_widget')
  VolumesWidget = require('views/dashboard/volumes_widget')

  class DashboardView extends Backbone.View
      template: templates['app/scripts/templates/dashboard/index.jst']
      className: 'stage'

      initialize: (opts) ->
        @volumes = opts.volumes
        @widgets = []
        @widgets.push new MiniWidget({title: 'Availability', value: 99.9, kind: 'percentage'})
        @widgets.push new MiniWidget({title: 'Backups', value: 'Excellent'})
        @widgets.push new MiniWidget({title: 'Recovery (DR)', value: 'Excellent'})
        @widgets.push new MiniWidget({title: 'Alerts', value: 0, kind: 'numeric', bar_class: 'green'})
        @widgets.push new CapacityWidget(volumes: @volumes)
        @widgets.push new VolumesWidget(volumes: @volumes, click: 'chart', title: 'Volumes')
        @widgets.push new StorageWidget({})

      render: ->
        @$el.html(@template())
        widget_html = (widget.render().el for widget in @widgets)
        this.$el.append(widget_html...)
        this

define (require, exports, modules) ->
  _ = require('underscore')
  Backbone = require('backbone')
  Volumes = require('collections/volumes')
  MenuView = require('views/menu_view')
  DashboardView = require('views/dashboard/dashboard_index_view')
  #VolumesView = require('views/volumes/volume_index_view')
  #VolumeItemView = require('views/volumes/volume_item_view')
  VolumeFormView = require('views/volumes/volume_form_view')

  class Router extends Backbone.Router
    routes:
      '': 'dashboard'
      'volumes': 'volumes'
      'volume/:volume_id': 'volume'
      'volume/:volume_id/edit': 'volumeEdit'

    initialize: ->
      @menu_view = new MenuView()
      $('nav#nav').html(@menu_view.render().el)
      @volumes = new Volumes()
      @volumes.enableLiveUpdate()
      @stage = []

    dashboard: ->
      @dashboard_view = @dashboard_view || new DashboardView(volumes: @volumes)
      @dashboard_view.stageLevel = 0
      @fetchVolumes =>
        @addToStage(@dashboard_view)
        window.vv = @volumes

    volumes: ->
      @volumes_view = @volumes_view || new Formation.Views.Volumes(volumes: @volumes)
      @volumes_view.stageLevel = 1

      $.magnificPopup.close()

      @fetchVolumes =>
        @addToStage(@volumes_view)

    volume: (volume_id)->
      @fetchVolumes =>
        @volume = @volumes.findWhere({id: +volume_id})
        volume_view = new Formation.Views.Volume(model: @volume)
        volume_view.stageLevel = 2
        @addToStage(volume_view)

    volumeEdit: (volume_id) ->
      @fetchVolumes =>
        @volume = @volumes.findWhere({id: +volume_id})
        volume_view = new Formation.Views.EditVolume(model: @volume)
        volume_view.stageLevel = 3
        @addToStage(volume_view)

    fetchVolumes: (callback) ->
      if @volumes.length > 0
        callback()
      else
        @volumes.fetch().complete =>
          callback()

    addToStage: (view) ->
      if @stage.length == 0
        view.className = 'stage'
        $('#stage').html(view.render().el)
        @stage.push(view)
      else
        last_view = @stage[@stage.length - 1]
        if last_view.stageLevel < view.stageLevel
          view.el.className = 'stage ondeck'
          $('#stage').append(view.render().el)
          last_view.$el.addClass('offstage')
          view.$el.removeClass('ondeck')
          @stage.push(view)

        # Going back to existing view
        else if @stage.length > 1 && @stage.indexOf(view) != -1
          # Remove intermediate views
          intermediate_views = @stage.slice(@stage.indexOf(view) + 1, @stage.length - 1 )
          for previous in intermediate_views
            previous.$el.remove()
            @stage.splice(@stage.indexOf(previous), 1)

          last_view.$el.addClass('ondeck')
          view.$el.removeClass('offstage')
          window.setTimeout( ->
            last_view.$el.remove()
          , 300)
          @stage.pop()

        # Going back and adding view
        else
          view.el.className = 'stage offstage'
          $('#stage').prepend(view.render().el)
          # For some reason this timeout is necessary
          window.setTimeout( ->
            last_view.$el.addClass('ondeck')
            view.$el.removeClass('offstage')
          , 5)

          @stage.unshift(view)
          existing_views = @stage.slice(1)
          window.setTimeout( =>
            for prev in existing_views
              prev.$el.remove()
              @stage.splice(@stage.indexOf(prev),1)
          , 300)




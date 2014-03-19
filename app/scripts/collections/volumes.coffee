define (require, exports, modules) ->
  _ = require('underscore')
  Backbone = require('backbone')
  Utils = require('utils')
  Volume = require('models/volume')

  class Volumes extends Backbone.Collection
    url: 'http://localhost:3000/mock_api/volumes'
    model: Volume

    enableLiveUpdate: ->
      @timer = window.setInterval(@fetchLive, 5000)

    disableLiveUpdate: ->
      window.clearInterval(@timer)

    fetchLive: =>
      $.getJSON(@.url, (data) =>
        for item in data
          vol = @get(item.id)
          vol.set(item) if vol
        @trigger('updated')
      )

    comparator: (obj) ->
      obj.get('priority')

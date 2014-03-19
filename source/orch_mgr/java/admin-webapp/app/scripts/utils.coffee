define (require, exports, modules) ->

  Utils =
    limit_colors:
      ['#68BC1D', '#B1D82F', '#E3D509']
    urls:
      server: ''
    addHeaders: (xhr) ->
      true #xhr.setRequestHeader('fds-key', '<%= ENV["FDS_KEY"] %>')
    setConnected: ->
      $('#connection').removeClass('disconnected')
    setDisconnected: ->
      $('#connection').addClass('disconnected')

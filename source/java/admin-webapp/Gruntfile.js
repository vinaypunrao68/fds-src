var LIVERELOAD_PORT = 35729;
//var SERVER_PORT = 9000;
var lrSnippet = require('connect-livereload')({port: LIVERELOAD_PORT});

var mountFolder = function (connect, dir) {
  return connect.static(require('path').resolve(dir));
};
var proxySnippet = require('grunt-connect-proxy/lib/utils').proxyRequest;

module.exports = function( grunt ){

    var proxySetting = {
        port: 7777,
        host: '127.0.0.1'
//        host: '10.3.91.145'
//        host: 'localhost'
    };

    var distDir = 'app';

    grunt.initConfig({
        pkg: grunt.file.readJSON( 'package.json' ),
        concat: {
            libs: {
                src: ['bower_components/angular/angular.js','bower_components/angular-translate/angular-translate.js', 'bower_components/angular-ui-router/release/angular-ui-router.js', 'bower_components/d3/d3.min.js'],
                dest: distDir + '/libs/dependencies.js'
            },
            release: {
                src: ['src/scripts/main.js','src/scripts/utils.js','src/scripts/enums.js','src/scripts/module-manifest.js','src/scripts/**/*.js','!src/scripts/directives/**', '!src/demo-scripts/**'],
                dest: 'temp/main.concat.js'
            }
        },
        sass: {
            dist: {
                files: {
                    'temp/main.css' : 'src/styles/sass/main.scss'
                }
            }
        },
        cssmin: {
            minify: {
                expand: true,
                cwd: 'temp',
                src: ['main.css'],
                dest: distDir + '/styles/',
                ext: '.min.css'
            }
        },
        uglify: {
            options: {
                mangle: false
            },
            release: {
                files: {
                    'temp/main.min.js' : [ 'temp/main.concat.js']
                }
            },
            directives: {
                files: [{
                    expand: true,
                    cwd: 'src/scripts/directives',
                    src: '**/*.js',
                    dest: distDir + '/scripts/directives'
                }]
            }
        },
        html2js: {
            main: {
                src: ['src/scripts/**/*.html'],
                dest: distDir + '/templates.js'
            }
        },
        copy: {
            libs: {
                files: [{
                    expand: true,
                    cwd: 'bower_components',
                    src: ['angular/angular.js', 'angular-ui-router/release/angular-ui-router.js', 'angular-translate/angular-translate.js', 'bootstrap/dist/css/bootstrap.min.css', 'jquery/dist/jquery.min.js','d3/d3.min.js', 'jquery/dist/jquery.min.map'],
                    dest: distDir + '/libs/'
                },
                {
                    expand: true,
                    cwd: 'src/styles',
                    src: 'flat-ui.*',
                    dest: distDir + '/libs/'
                },
                {
                    expand: true,
                    cwd: 'src/styles',
                    src: 'fonts/**',
                    dest: distDir
                }]
            },
            dev: {
                files: [{
                    expand: true,
                    cwd: 'src',
                    src: ['**/*.js', 'images/**/*'],
//                    src: ['**/*.html', '**/*.js', 'images/**/*'],
                    dest: distDir + '/'
                },
                {
                    expand: true,
                    cwd: 'src',
                    src: ['om*html'],
                    dest: distDir + '/'
                }]
            },
            release: {
                files: [{
                    expand: true,
                    cwd: 'temp',
                    src: ['main.min.js'],
                    dest: distDir + '/scripts/'
                },
                {
                    expand: true,
                    cwd: 'src',
                    src: ['images/**/*','scripts/services/data/fake*.js','!index.html'],
//                    src: ['**/*.html', 'images/**/*','scripts/services/data/fake*.js','!index.html'],
                    dest: distDir + '/'
                },
                {
                    expand: true,
                    cwd: 'src',
                    src: ['robots.txt'],
                    dest: distDir + '/'
                }]
            },
            demo: {
                files: [{
                    expand: true,
                    cwd: 'test/e2e',
                    src: ['mock*js'],
                    dest: distDir + '/'
                }]
            }
        },
        replace: {
            release: {
                src: 'src/index.html',
                dest: distDir + '/index.html',
                replacements: [{
                    from: '<!--@@RELEASE@@',
                    to: ''
                },
               {
                   from: '@@RELEASE@@-->',
                   to: ''
               }]
            },
            dev: {
                src: 'src/index.html',
                dest: distDir + '/index.html',
                replacements: [{
                    from: '<!--@@DEV@@',
                    to: ''
                },
                {
                   from: '@@DEV@@-->',
                   to: ''
                }]
            },
            demo: {
                src: 'src/index.html',
                dest: distDir + '/index.html',
                replacements: [{
                    from: '<!--@@DEV@@',
                    to: ''
                },
                {
                   from: '@@DEV@@-->',
                   to: ''
                },
                {
                    from: '<!--@@DEMO@@',
                    to: ''
                },
                {
                    from: '@@DEMO@@-->',
                    to: ''
                }]                
            }
        },
        karma: {
            unit: {
                configFile: 'karma.conf.js'
            }
        },
        protractor_webdriver: {

        },
        protractor: {
            options: {
                configFile: 'e2e.conf.js',
                keepAlive: true,
                noColor: false,
                args: { seleniumPort: 4444 },
                debug: false
            },
            singlerun: {}
        },
        clean: {
            all: ['app'],
            temp: ['temp']
        },
        connect: {
            options: {
                base: './' + distDir,
                port: 9001,
                hostname: 'localhost',
            },
            proxies: [{
                context: '/fds',
                host: proxySetting.host,
                port: proxySetting.port,
                https: false,
                changeOrigin: false,
                xforward: false
            }],
            livereload: {
                options: {
                    middleware: function (connect) {
                        var middlewares =  [
                            proxySnippet,
                            lrSnippet,
                            mountFolder(connect, './' + distDir ),
                        ];
                        return middlewares;
                    }
                }
            }
        },
        watch: {
            scripts: {
                files: ['src/**/*.js'],
                tasks: ['copy:dev','replace:dev','sass:dist', 'cssmin', 'html2js', 'clean:temp'],
                options: {
                    spawn: false
                }
            },
            html: {
                files: ['src/**/*.html'],
                tasks: ['copy:dev', 'replace:dev', 'sass:dist', 'cssmin', 'html2js', 'clean:temp' ],
                options: {
                    spawn: false
                }
            },
            css: {
                files: ['src/**/*.scss', 'src/**/*.css'],
                tasks: ['copy:dev', 'replace:dev', 'sass:dist', 'cssmin', 'clean:temp'],
                options: {
                    spawn: false
                }
            },
            demo: {
                files: ['src/**/*.js', 'src/**/*.html', 'src/**/*.scss', 'src/**/*.css'],
                tasks: ['copy:dev', 'copy:demo', 'replace:dev', 'replace:demo', 'sass:dist', 'cssmin', 'html2js', 'clean:temp'],
                options: {
                    spawn: false
                }
            }
        },
        focus: {
            dev: {
                include: ['scripts', 'html', 'css']
            },
            demo: {
                include: [ 'demo' ]
            }
        }
    });

    grunt.loadNpmTasks( 'grunt-contrib-concat' );
    grunt.loadNpmTasks( 'grunt-contrib-clean' );
    grunt.loadNpmTasks( 'grunt-contrib-sass' );
    grunt.loadNpmTasks( 'grunt-contrib-copy' );
    grunt.loadNpmTasks( 'grunt-contrib-connect' );
    grunt.loadNpmTasks( 'grunt-connect-proxy' );
    grunt.loadNpmTasks( 'grunt-focus');
    grunt.loadNpmTasks( 'grunt-contrib-watch' );
    grunt.loadNpmTasks( 'connect-livereload' );
    grunt.loadNpmTasks( 'grunt-karma' );
    grunt.loadNpmTasks( 'grunt-protractor-runner' );
    grunt.loadNpmTasks( 'grunt-contrib-cssmin' );
    grunt.loadNpmTasks( 'grunt-contrib-uglify' );
    grunt.loadNpmTasks( 'grunt-text-replace' );
    grunt.loadNpmTasks( 'grunt-html2js' );

    grunt.registerTask( 'default', ['clean:all','copy:libs','copy:dev','replace:dev','sass:dist','cssmin','html2js','clean:temp'] );
    grunt.registerTask( 'dev', ['default'] );

    grunt.registerTask( 'release', ['clean:all','copy:libs','replace:release','concat:release','uglify','uglify:directives','copy:release','sass:dist','cssmin','html2js','clean:temp'] );

    grunt.registerTask( 'serve', ['default', 'configureProxies:server', 'connect:livereload', 'focus:dev'] );
    grunt.registerTask( 'release-serve', ['release', 'configureProxies:server', 'connect:livereload', 'focus:dev'] );
    grunt.registerTask( 'demo', ['clean:all', 'copy:libs', 'copy:dev', 'copy:demo', 'replace:demo', 'sass:dist', 'cssmin', 'html2js', 'clean:temp', 'connect:livereload', 'focus:demo' ] );
    grunt.registerTask( 'e2e', ['default','protractor'] );
    grunt.registerTask( 'unittest', ['default', 'karma' ] );
};

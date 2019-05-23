# tests and condition when to skip the test
ges_tests = [
    {'file': 'ges/asset'},
    {'file': 'ges/backgroundsource'},
    {'file': 'ges/basic'},
    {'file': 'ges/layer'},
    {'file': 'ges/effects'},
    {'file': 'ges/uriclip'},
    {'file': 'ges/clip'},
    {'file': 'ges/timelineedition'},
    {'file': 'ges/titles'},
    {'file': 'ges/transition'},
    {'file': 'ges/overlays'},
    {'file': 'ges/mixers'},
    {'file': 'ges/group'},
    {'file': 'ges/project'},
    {'file': 'ges/track'},
    {'file': 'ges/tempochange'},
    {'file': 'ges/timecode', 'objects': libges.extract_objects('ges-time-code.c')},
    {'file': 'ges/negative'},
    {'file': 'nle/simple'},
    {'file': 'nle/complex'},
    {'file': 'nle/nleoperation'},
    {'file': 'nle/nlecomposition'},
    {'file': 'nle/tempochange'},
]

test_defines = [
  '-UG_DISABLE_ASSERT',
  '-UG_DISABLE_CAST_CHECKS',
  '-DGES_TEST_FILES_PATH="' + meson.current_source_dir() + '/ges/"',
  '-DGST_CHECK_TEST_ENVIRONMENT_BEACON="GST_STATE_IGNORE_ELEMENTS"',
  '-DTESTFILE="' + meson.current_source_dir() + '/meson.build"',
  '-DGST_USE_UNSTABLE_API',
]

pluginsdirs = []
if gst_dep.type_name() == 'pkgconfig'
  pbase = dependency('gstreamer-plugins-base-' + apiversion, required : false)
  pbad = dependency('gstreamer-plugins-bad-' + apiversion, required : false)

  pluginsdirs = [gst_dep.get_pkgconfig_variable('pluginsdir'),
                 pbase.get_pkgconfig_variable('pluginsdir'),
                 pbad.get_pkgconfig_variable('pluginsdir')]
endif

foreach t : ges_tests
  fname = '@0@.c'.format(t.get('file'))
  test_name = t.get('file').underscorify()
  skip_test = t.get('skip', false)
  objects = t.get('objects', [])

  if not skip_test
    env = environment()
    env.set('GST_PLUGIN_SYSTEM_PATH_1_0', '')
    env.set('GST_STATE_IGNORE_ELEMENTS', '')
    env.set('CK_DEFAULT_TIMEOUT', '20')
    env.set('GST_REGISTRY', '@0@/@1@.registry'.format(meson.current_build_dir(), test_name))
    env.set('GST_PLUGIN_PATH_1_0', [meson.build_root()] + pluginsdirs)

    exe = executable(test_name, fname,
        'ges/test-utils.c', 'nle/common.c',
        c_args : ges_c_args + test_defines,
        include_directories : [configinc],
        dependencies : libges_deps + [gstcheck_dep, ges_dep],
        objects: objects
    )
    test(test_name, exe, env: env, timeout : 3 * 60)
  endif
endforeach

if build_gir
  # Make sure to use the subproject gst-validate-launcher if avalaible.
  if gstvalidate_dep.found() and gstvalidate_dep.type_name() == 'internal'
    runtests = subproject('gst-devtools').get_variable('launcher')
  else
    runtests = find_program('gst-validate-launcher', required : false)
  endif

  if runtests.found()
    test('pythontests', runtests, args: ['--pyunittest-dir', meson.current_source_dir(), 'pyunittest', '--dump-on-failure'],
         env: env)
  endif
endif

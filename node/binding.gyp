{
  'targets': [
    {
      'target_name': 'stash',
      'include_dirs': [
          'lib',
        ],
      'dependencies': [ 'lib/mustache.gyp:mustache' ],
      'sources': [
        'stash.cc',
        'stash_template.cc' ]
    }
  ]
}

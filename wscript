def options(opt):
  opt.tool_options('compiler_c ruby')

def configure(conf):
  conf.recurse('lib')

def build(bld):
  bld.recurse('lib')

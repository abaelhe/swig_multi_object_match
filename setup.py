from distutils.core import setup
from Cython.Distutils import build_ext

from distutils.core import setup, Extension
import os,sys

# if sys.platform.startswith('linux'):
#     os_version = os.popen('cat /proc/version').read().upper()
#     if os_version.find('UBUNTU') > 0:
#         os.system('sudo apt-get install -y autoconf')
#     elif os_version.find('REDHAT') > 0 or os_version.find('CENTOS'):
#         os.system('sudo yum install -y autoconf')
#     else:
#         raise SystemError('current ONLY support Red Hat | CentOS | Ubuntu Linux distributions.')
#     os.system('pushd jemalloc;sh autogen.sh && ./configure --without-export && make -j4;popd')
#
setup(name='mmapfile',
      version='1.0',
      author='Abael He<abaelhe@icloud.com>',
      ext_modules=[
          Extension("_" + "mmapfile",
                    language="c++",
                    swig_opts=[
                        '-c++',
                        '-small',
                    ],
                    undef_macros=[
#                        'DEBUG',
                    ],
                    define_macros=[

#                        ('DEBUG', 1),
                    ],
                    extra_compile_args = ['-std=c++11', '-fPIC', '-rdynamic', '-ggdb', '-O0'] + (['-m64',]  if sys.platform.startswith('linux') else []), # '-ggdb', '-O0'],
                    sources=["modules/mmapfile.i",
                             "modules/driver.cpp",
                             "modules/mmapfile.cpp",
                             "modules/linuxplatform.cpp",
                    ],
                    include_dirs=[".", "modules", "/usr/local/include"],
                    library_dirs=['/usr/local/lib', 'modules'],
                    #extra_objects=["jemalloc/lib/libjemalloc.a"],
                    runtime_library_dirs=['.', '/usr/local/lib'],
                    libraries=['m','dl', 'rt', 'pthread'] if sys.platform.startswith('linux') else [])
      ],
      cmdclass={'build_ext': build_ext},
      )

os.system('find ./build -type f -name "*.so" -exec cp -vf \{\} ./modules/ \; ')

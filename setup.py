from distutils.core import setup
from Cython.Distutils import build_ext

from distutils.core import setup, Extension
import os,sys
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
                    extra_compile_args = ['-std=c++11', '-O3'], # '-ggdb', '-O0'],
                    sources=["modules/mmapfile.i",
                             "modules/driver.cpp",
                             "modules/mmapfile.cpp",
                             "modules/linuxplatform.cpp",
                    ],
                    include_dirs=[".", "modules", "/usr/local/include"],
                    library_dirs=['/usr/local/lib', 'modules'],

                    extra_objects=[],
                    runtime_library_dirs=['.', '/usr/local/lib'],
                    libraries=['rt','pthread'] if sys.platform.startswith('linux') else [])
      ],
      cmdclass={'build_ext': build_ext},
      )

os.system('find ./build -type f -name "*.so" -exec cp -vf \{\} ./modules/ \; ')
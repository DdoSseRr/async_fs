import os
import subprocess
from distutils.ccompiler import new_compiler
from distutils.errors import CompileError
from distutils.sysconfig import customize_compiler

from setuptools import Extension, setup


def check_for_liburing():
    try:
        result = subprocess.run(['pkg-config', '--exists', 'liburing'], check=True)  # noqa: S607
        return result.returncode == 0  # noqa: TRY300
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False

def check_for_liburing_headers():
    compiler = new_compiler()
    customize_compiler(compiler)
    test_file = 'test.c'
    with open(test_file, 'w') as f:
        f.write('#include <liburing.h>\n')
    try:
        compiler.compile([test_file])
        return True
    except CompileError:
        return False
    finally:
        os.remove(test_file)


if not check_for_liburing_headers():
    raise CompileError("liburing is not installed or its headers are missing. Please install them using your package manager.")

module = Extension(
    'async_fs',
    sources=['async_fs/async_fs.cpp'],
    libraries=['uring'],
    extra_compile_args=['-std=c++11'],
)

setup(
    name='async-fs',
    version='0.0.1',
    description='Asynchronous file system operations using io_uring',
    long_description=open('README.md').read(),
    long_description_content_type='text/markdown',
    ext_modules=[module],
    author='DdoSseRr',
    author_email='ddosserr.developer.666@gmail.com',
    url='https://github.com/DdoSseRr/async_fs',
    classifiers=[
        'Programming Language :: Python :: 3.12',
        'Programming Language :: C++',
        'License :: OSI Approved :: MIT License',
        'Operating System :: POSIX :: Linux',
        'Operating System :: Unix',
    ],
    platforms=['Linux'],
    python_requires='>=3.11',
    license='MIT',
)
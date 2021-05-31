import argparse

from configuration import load_compiler_configuration

SINGLE_COMMAND_SEPARATOR = ' \\\n    && '
LAYER_TO_IMAGE_TAG_MAP = {'os': 'preimage1', 'boost': 'preimage2', 'deps': 'preimage3', 'test': '', 'conan': 'conan'}


def print_line(lines, **kwargs):
    print(SINGLE_COMMAND_SEPARATOR.join(lines).format(**kwargs))


def print_line_with_continuation(lines, **kwargs):
    print(SINGLE_COMMAND_SEPARATOR.join(lines).format(**kwargs), end='')
    print(SINGLE_COMMAND_SEPARATOR, end='')


def print_lines(lines, **kwargs):
    print('\n'.join(lines).format(**kwargs))


def format_multivalue_options(key, values):
    return '{}=\'{}\''.format(key, ' '.join(values))


# region OptionsManager

class OptionsManager:
    class OptionsDescriptor:
        def __init__(self):
            self.options = []
            self.cxxflags = []
            self.linkflags = []
            self.sanitizer = None

    def __init__(self, compiler_configuration, operating_system, versions_filepath):
        self.compiler = compiler_configuration.compiler
        self.operating_system = operating_system
        self.sanitizers = compiler_configuration.sanitizers
        self.architecture = compiler_configuration.architecture
        self.stl = compiler_configuration.stl

        self.versions = {}
        with (open(versions_filepath, 'rt')) as infile:
            for line in infile.readlines():
                line_parts = line.strip().split(' = ')
                if 2 == len(line_parts):
                    self.versions[line_parts[0]] = line_parts[1]

    @property
    def is_clang(self):
        return self.compiler.c.startswith('clang')

    @property
    def base_image_name(self):
        name_parts = [self.operating_system, self.compiler.c, str(self.compiler.version)]
        return 'symbolplatform/symbol-server-compiler:{}'.format('-'.join(name_parts))

    def layer_image_name(self, layer):
        name_parts = [self.operating_system, self.compiler.c, str(self.compiler.version)]
        if 'conan' != layer:
            name_parts.extend(self.sanitizers + [self.architecture])

        tag = LAYER_TO_IMAGE_TAG_MAP[layer]
        if tag:
            name_parts.append(tag)

        return 'symbolplatform/symbol-server-build-base:{}'.format('-'.join(name_parts))

    def bootstrap(self):
        options = []
        if self.is_clang:
            options += ['--with-toolset=clang']

        return options

    def b2(self):  # pylint: disable=invalid-name
        options = []
        cxxflags = [self._arch_flag]
        if self.is_clang:
            options += ['toolset=clang']
            options += [format_multivalue_options('linkflags', ['-stdlib={}'.format(self.stl.lib)])]
            cxxflags += self._stl_flags

        options += [format_multivalue_options('cxxflags', cxxflags)]
        return options

    def openssl(self):
        return [format_multivalue_options('CFLAGS', [self._arch_flag])]

    def openssl_configure(self):
        if 'address' in self.sanitizers:
            return ['enable-asan']

        if 'undefined' in self.sanitizers:
            return ['enable-ubsan']

        return []

    def mongo_c(self):
        descriptor = self.OptionsDescriptor()
        descriptor.options += [
            '-DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF',
            '-DENABLE_MONGODB_AWS_AUTH=OFF',
            '-DENABLE_TESTS=OFF',
            '-DENABLE_EXAMPLES=OFF',
            '-DENABLE_SASL=OFF'
        ]
        return self._cmake(descriptor)

    def mongo_cxx(self):
        descriptor = self.OptionsDescriptor()
        descriptor.options += ['-DCMAKE_CXX_STANDARD=17']
        return self._cmake(descriptor)

    def libzmq(self):
        descriptor = self._zmq_descriptor()
        descriptor.options += ['-DWITH_TLS=OFF']

        if self.is_clang:
            # Xeon-based build machine, even with -mskylake seems to do miscompilation in libzmq,
            # try to pass additional flags to disable faulty optimizations
            descriptor.cxxflags += ['-mno-avx', '-mno-avx2']

        return self._cmake(descriptor)

    def cppzmq(self):
        descriptor = self._zmq_descriptor()
        descriptor.options += ['-DCPPZMQ_BUILD_TESTS=OFF']
        return self._cmake(descriptor)

    def _zmq_descriptor(self):
        descriptor = self.OptionsDescriptor()
        if 'thread' in self.sanitizers:
            descriptor.sanitizer = 'thread'

        return descriptor

    def rocks(self):
        descriptor = self.OptionsDescriptor()
        descriptor.options += [
            '-DPORTABLE=1',
            '-DWITH_TESTS=OFF',
            '-DWITH_TOOLS=OFF',
            '-DWITH_BENCHMARK_TOOLS=OFF',
            '-DWITH_CORE_TOOLS=OFF',
            '-DWITH_GFLAGS=OFF'
        ]

        if 'undefined' in self.sanitizers:
            descriptor.options += ['-DUSE_RTTI=1']

        return self._cmake(descriptor)

    def googletest(self):
        descriptor = self.OptionsDescriptor()
        descriptor.options += ['-DCMAKE_POSITION_INDEPENDENT_CODE=ON']
        descriptor.sanitizer = ','.join(self.sanitizers)
        return self._cmake(descriptor)

    def googlebench(self):
        descriptor = self.OptionsDescriptor()
        descriptor.options += ['-DBENCHMARK_ENABLE_GTEST_TESTS=OFF']
        return self._cmake(descriptor)

    @property
    def _arch_flag(self):
        return '-march={}'.format(self.architecture)

    @property
    def _stl_flags(self):
        return None if not self.stl else ['-std={}'.format(self.stl.version), '-stdlib={}'.format(self.stl.lib)]

    def _cmake(self, descriptor):
        descriptor.options += [
            '-DCMAKE_BUILD_TYPE=Release',
            '-DCMAKE_INSTALL_PREFIX=/usr',
        ]
        descriptor.cxxflags += [self._arch_flag]

        if self.is_clang:
            descriptor.options += [format_multivalue_options('-DCMAKE_CXX_COMPILER', [self.compiler.cpp])]
            descriptor.cxxflags += self._stl_flags

        if descriptor.sanitizer:
            sanitize_flag = '-fsanitize={}'.format(descriptor.sanitizer)
            descriptor.options += [format_multivalue_options('-DCMAKE_C_FLAGS', [sanitize_flag])]
            descriptor.cxxflags += [sanitize_flag]
            descriptor.linkflags += [sanitize_flag]

        if descriptor.cxxflags:
            descriptor.options += [format_multivalue_options('-DCMAKE_CXX_FLAGS', descriptor.cxxflags)]

        if descriptor.linkflags:
            descriptor.options += [
                format_multivalue_options('-DCMAKE_SHARED_LINKER_FLAGS', descriptor.linkflags),
                format_multivalue_options('-DCMAKE_EXE_LINKER_FLAGS', descriptor.linkflags)
            ]

        return descriptor.options


# endregion

# region SYSTEMS

class UbuntuSystem:
    @staticmethod
    def add_base_os_packages():
        apt_packages = [
            'autoconf',
            'ca-certificates',
            'ccache',
            'curl',
            'gdb',
            'git',
            'libatomic-ops-dev',
            'libgflags-dev',
            'libsnappy-dev',
            'libtool',
            'libunwind-dev',
            'make',
            'ninja-build',
            'pkg-config',
            'python3',
            'python3-ply',
            'xz-utils'
        ]
        print_line_with_continuation([
            'RUN apt-get -y update',
            'apt-get install -y {APT_PACKAGES}',
            'rm -rf /var/lib/apt/lists/*'
        ], APT_PACKAGES=' '.join(apt_packages))

    @staticmethod
    def add_test_packages(install_openssl):
        apt_packages = ['python3-pip']
        if install_openssl:
            apt_packages += ['libssl-dev']

        print_line([
            'RUN apt-get -y update',
            'apt-get remove -y --purge pylint',
            'apt-get install -y {APT_PACKAGES}',
            'python3 -m pip install -U pycodestyle pylint pyyaml'
        ], APT_PACKAGES=' '.join(apt_packages))


class FedoraSystem:
    @staticmethod
    def add_base_os_packages():
        rpm_packages = [
            'ccache',
            'curl',
            'gdb',
            'gflags-devel',
            'git',
            'libunwind-devel',
            'make',
            'ninja-build',
            'python3',
            'xz'
        ]
        print_line_with_continuation([
            'RUN dnf update --assumeyes',
            'dnf install --assumeyes {RPM_PACKAGES}',
            'dnf clean all',
            'rm -rf /var/cache/yum'
        ], RPM_PACKAGES=' '.join(rpm_packages))

    @staticmethod
    def add_test_packages(install_openssl):
        rpm_packages = ['python3-pip']
        if install_openssl:
            rpm_packages += ['openssl-devel']

        print_line([
            'RUN dnf update --assumeyes',
            'dnf remove --assumeyes pylint',
            'dnf install --assumeyes {RPM_PACKAGES}',
            'python3 -m pip install -U pycodestyle pylint pyyaml',
            'dnf clean all',
            'rm -rf /var/cache/yum'
        ], RPM_PACKAGES=' '.join(rpm_packages))


SYSTEMS = {'ubuntu': UbuntuSystem, 'fedora': FedoraSystem}


def generate_phase_os(options):
    print_lines([
        'FROM {BASE_IMAGE_NAME}',
        'ARG DEBIAN_FRONTEND=noninteractive',
        'MAINTAINER Catapult Development Team'
    ], BASE_IMAGE_NAME=options.base_image_name)

    SYSTEMS[options.operating_system].add_base_os_packages()

    cmake_version = options.versions['cmake']
    cmake_script = 'cmake-{}-Linux-x86_64.sh'.format(cmake_version)
    cmake_uri = 'https://github.com/Kitware/CMake/releases/download/v{}'.format(cmake_version)
    print_line([
        'curl -o {CMAKE_SCRIPT} -SL "{CMAKE_URI}/{CMAKE_SCRIPT}"',
        'chmod +x {CMAKE_SCRIPT}',
        './{CMAKE_SCRIPT} --skip-license --prefix=/usr',
        'rm -rf {CMAKE_SCRIPT}'
    ], CMAKE_SCRIPT=cmake_script, CMAKE_URI=cmake_uri)


def generate_phase_boost(options):
    print('FROM {}'.format(options.layer_image_name('os')))
    gosu_version = options.versions['gosu']
    gosu_target = '/usr/local/bin/gosu'
    gosu_uri = 'https://github.com/tianon/gosu/releases/download/{}'.format(gosu_version)
    print_line([
        'RUN curl -o {GOSU_TARGET} -SL "{GOSU_URI}/gosu-$(dpkg --print-architecture)"',
        'chmod +x {GOSU_TARGET}'
    ], GOSU_TARGET=gosu_target, GOSU_URI=gosu_uri)

    boost_version = options.versions['boost']
    boost_disabled_libs = map('--without-{}'.format, [
        'context',
        'contract',
        'coroutine',
        'fiber',
        'graph',
        'graph_parallel',
        'headers',
        'iostreams',
        'json',
        'mpi',
        'nowide',
        'python',
        'serialization',
        'stacktrace',
        'test',
        'timer',
        'type_erasure',
        'wave'
    ])

    print_args = {
        'BOOST_ARCHIVE': 'boost_1_{}_0'.format(boost_version),
        'BOOST_URI': 'https://boostorg.jfrog.io/artifactory/main/release/1.{}.0/source'.format(boost_version),
        'BOOTSTRAP_OPTIONS': ' '.join(options.bootstrap()),
        'B2_OPTIONS': ' '.join(options.b2()),
        'BOOST_DISABLED_LIBS': ' '.join(boost_disabled_libs)
    }
    print_line([
        'RUN curl -o {BOOST_ARCHIVE}.tar.gz -SL {BOOST_URI}/{BOOST_ARCHIVE}.tar.gz',
        'tar -xzf {BOOST_ARCHIVE}.tar.gz',
        'mkdir /mybuild',
        'cd {BOOST_ARCHIVE}',
        './bootstrap.sh {BOOTSTRAP_OPTIONS} --prefix=/mybuild',
        './b2 {B2_OPTIONS} --prefix=/mybuild {BOOST_DISABLED_LIBS} -j 8 stage release',
        './b2 {B2_OPTIONS} {BOOST_DISABLED_LIBS} install'
    ], **print_args)


def add_git_dependency(organization, project, versions_map, options, revision=1):
    version = versions_map['{}_{}'.format(organization, project)]
    print_line([
        'RUN git clone https://github.com/{ORGANIZATION}/{PROJECT}.git',
        'cd {PROJECT}',
        'git checkout {VERSION}',
        'mkdir _build',
        'cd _build',
        'cmake {OPTIONS} ..',
        'make -j 8',
        'make install',
        'cd ..',
        'rm -rf {PROJECT}',
        'echo \"force rebuild revision {REVISION}\"'
    ], ORGANIZATION=organization, PROJECT=project, VERSION=version, OPTIONS=' '.join(options), REVISION=revision)


def generate_phase_deps(options):
    print('FROM {}'.format(options.layer_image_name('boost')))
    add_git_dependency('mongodb', 'mongo-c-driver', options.versions, options.mongo_c())
    add_git_dependency('mongodb', 'mongo-cxx-driver', options.versions, options.mongo_cxx())

    add_git_dependency('zeromq', 'libzmq', options.versions, options.libzmq())
    add_git_dependency('zeromq', 'cppzmq', options.versions, options.cppzmq())

    add_git_dependency('facebook', 'rocksdb', options.versions, options.rocks())


def generate_phase_test(options):
    print('FROM {}'.format(options.layer_image_name('deps')))
    add_git_dependency('google', 'googletest', options.versions, options.googletest())
    add_git_dependency('google', 'benchmark', options.versions, options.googlebench())

    SYSTEMS[options.operating_system].add_test_packages(not options.sanitizers)

    if options.sanitizers:
        print_line([
            'RUN git clone https://github.com/openssl/openssl.git',
            'cd openssl',
            'git checkout OpenSSL_1_1_1g',
            '{OPEN_SSL_OPTIONS} perl ./Configure linux-x86_64-clang {OPEN_SSL_CONFIGURE} --prefix=/usr/local --openssldir=/usr/local',
            'make -j 8',
            'make install',
            'cd ..',
            'rm -rf openssl'
        ], OPEN_SSL_OPTIONS=' '.join(options.openssl()), OPEN_SSL_CONFIGURE=' '.join(options.openssl_configure()))

    print_lines([
        'RUN echo "docker image build $BUILD_NUMBER"',
        'CMD ["/bin/bash"]'
    ])


def generate_phase_conan(options):
    print('FROM {}'.format(options.layer_image_name('os')))

    apt_packages = ['python3-pip']

    print_line([
        'RUN apt-get -y update',
        'apt-get install -y {APT_PACKAGES}',
        'python3 -m pip install -U "conan>=1.33.0"'
    ], APT_PACKAGES=' '.join(apt_packages))


def main():
    parser = argparse.ArgumentParser(description='catapult base image dockerfile generator')
    parser.add_argument('--layer', help='name of docker layer to generate', choices=LAYER_TO_IMAGE_TAG_MAP.keys(), required=True)
    parser.add_argument('--compiler-configuration', help='path to compiler configuration yaml', required=True)
    parser.add_argument('--operating-system', help='operating system', required=True)
    parser.add_argument('--versions', help='locked versions file', required=True)
    parser.add_argument('--name-only', help='true to output layer name', action='store_true')
    args = parser.parse_args()

    compiler_configuration = load_compiler_configuration(args.compiler_configuration)
    options_manager = OptionsManager(compiler_configuration, args.operating_system, args.versions)

    if args.name_only:
        print(options_manager.layer_image_name(args.layer))
        return

    options = options_manager
    {
        'os': generate_phase_os,
        'boost': generate_phase_boost,
        'deps': generate_phase_deps,
        'test': generate_phase_test,

        'conan': generate_phase_conan
    }[args.layer](options)


if __name__ == '__main__':
    main()

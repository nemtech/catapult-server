import argparse
import os
import sys
from pathlib import Path

from BasicBuildManager import BasicBuildManager
from environment import EnvironmentManager
from process import ProcessManager

CONAN_NEMTECH_REMOTE = 'https://catapult.jfrog.io/artifactory/api/conan/ngl-conan'


class BuildEnvironment:
    def __init__(self, use_conan, process_manager, environment_manager):
        self.use_conan = use_conan
        self.dispatch_subprocess = process_manager.dispatch_subprocess
        self.environment_manager = environment_manager

    def prepare(self):
        self._prepare_directory()
        self._prepare_environment_variables()

        if 'win32' != sys.platform:
            self.dispatch_subprocess(['ccache', '-M', '30G'])
            self.dispatch_subprocess(['ccache', '-s'])

    def _prepare_directory(self):
        self.environment_manager.mkdirs('/tmp/_build')
        self.environment_manager.chdir('/tmp/_build')

    def _prepare_environment_variables(self):
        if self.use_conan:
            self.environment_manager.set_env_var('HOME', '/conan')  # conan cache directory
            #if 'win32' == sys.platform:
            #self.environment_manager.set_env_var('USERPROFILE', 'c:\\conan')  # conan cache directory
        else:
            self.environment_manager.set_env_var('BOOST_ROOT', '/mybuild')
            self.environment_manager.set_env_var('GTEST_ROOT', '/usr/local')

    def prepare_conan(self, settings):
        # create default profile if it does not exist
        if self.dispatch_subprocess(['conan', 'profile', 'get', 'settings.compiler', 'default'], show_output=False, handle_error=False):
            self.dispatch_subprocess(['conan', 'profile', 'new', 'default', '--detect'])

        self.dispatch_subprocess(['conan', 'remote', 'add', '--force', 'nemtech', CONAN_NEMTECH_REMOTE])
        self.dispatch_subprocess(['conan', 'config', 'set', 'general.revisions_enabled=True'])

        for key, value in settings.items():
            self.dispatch_subprocess(['conan', 'profile', 'update', 'settings.compiler.{}={}'.format(key, value), 'default'])

    def run_conan_install(self):
        # assuming working directory == build directory
        self.dispatch_subprocess(['conan', 'install', '/catapult-src', '--build', 'missing'])


class BuildManager(BasicBuildManager):
    def __init__(self, args, process_manager, environment_manager):
        super().__init__(args.compiler_configuration, args.build_configuration)
        self.dispatch_subprocess = process_manager.dispatch_subprocess
        self.environment_manager = environment_manager

    def cmake_settings(self):
        settings = [
            ('CMAKE_INSTALL_PREFIX', '/binaries'),
            ('CMAKE_BUILD_TYPE', self.build_configuration),
            ('CATAPULT_TEST_DB_URL', 'mongodb://db:27017'),
            ('CATAPULT_DOCKER_TESTS', 'ON'),
            ('ENABLE_CODE_COVERAGE', 'OFF'),
            ('ARCHITECTURE_NAME', self.architecture)
        ]

        if self.enable_diagnostics:
            settings.append(('ENABLE_CATAPULT_DIAGNOSTICS', 'ON'))

        if self.use_conan:
            settings.append(('USE_CONAN', 'ON'))

        if self.sanitizers:
            settings.extend([
                ('USE_SANITIZER', ','.join(self.sanitizers)),
                ('OPENSSL_ROOT_DIR', '/usr/local')
            ])

        if self.is_release:
            settings.append(('CATAPULT_BUILD_RELEASE', 'ON'))
            settings.append(('ENABLE_TESTS', 'OFF'))

        if 'public' == self.build_disposition:
            settings.append(('CATAPULT_BUILD_RELEASE_PUBLIC', 'ON'))

        return ['-D{}={}'.format(key, value) for key, value in settings]

    def run_cmake(self):
        cmake_settings = self.cmake_settings()
        if 'win32' == sys.platform:
            self.dispatch_subprocess(['cmake'] + cmake_settings + ['-G', 'Visual Studio 16 2019', '-A', 'x64', '/catapult-src'])
        else:
            self.dispatch_subprocess(['cmake'] + cmake_settings + ['-G', 'Ninja', '/catapult-src'])

    def build(self):
        if 'win32' == sys.platform:
            pass
        else:
            self.dispatch_subprocess(['ninja', 'publish'])
            self.dispatch_subprocess(['ninja'])
            self.dispatch_subprocess(['ninja', 'install'])

    def copy_dependencies(self, destination):
        if self.use_conan:
            self.environment_manager.copy_tree_with_symlinks('./deps', destination)
            return

        self.environment_manager.mkdirs(destination)
        for name in ['atomic', 'chrono', 'date_time', 'filesystem', 'log', 'log_setup', 'program_options', 'regex', 'thread']:
            self.environment_manager.copy_glob_with_symlinks('/mybuild/lib', 'libboost_{}.so*'.format(name), destination)

        for name in ['bson-1.0', 'mongoc-1.0', 'bsoncxx', 'mongocxx', 'zmq', 'rocksdb', 'snappy', 'gflags']:
            system_bin_path = self.environment_manager.system_bin_path
            self.environment_manager.copy_glob_with_symlinks(system_bin_path, 'lib{}.so*'.format(name), destination)

    def copy_compiler_deps(self, destination):
        for dependency_pattern in self.compiler.deps:
            directory_path = os.path.dirname(dependency_pattern)
            pattern = os.path.basename(dependency_pattern)
            self.environment_manager.copy_glob_with_symlinks(directory_path, pattern, destination)

    def copy_sanitizer_deps(self, destination):
        for name in ['crypto', 'ssl']:
            self.environment_manager.copy_glob_with_symlinks('/usr/local/lib/', 'lib{}*'.format(name), destination)

        self.environment_manager.copy_tree_with_symlinks('/usr/local/lib/engines-1.1', Path(destination) / 'engines-1.1')

    def copy_files(self):
        deps_output_path = '/binaries/deps'
        tests_output_path = '/binaries/tests'

        # copy deps
        self.copy_dependencies(deps_output_path)
        self.copy_compiler_deps(deps_output_path)

        if self.sanitizers:
            self.copy_sanitizer_deps(deps_output_path)

        # copy tests
        if not self.is_release:
            self.environment_manager.mkdirs(tests_output_path)
            self.environment_manager.copy_glob_with_symlinks('./bin', 'tests*', tests_output_path)

        # list directories
        self.dispatch_subprocess(['ls', '-alh', deps_output_path])

        if not self.is_release:
            self.dispatch_subprocess(['ls', '-alh', tests_output_path])


def main():
    parser = argparse.ArgumentParser(description='catapult project build generator')
    parser.add_argument('--compiler-configuration', help='path to compiler configuration yaml', required=True)
    parser.add_argument('--build-configuration', help='path to build configuration yaml', required=True)
    parser.add_argument('--dry-run', help='outputs desired commands without runing them', action='store_true')
    args = parser.parse_args()

    process_manager = ProcessManager(args.dry_run)
    environment_manager = EnvironmentManager(args.dry_run)

    builder = BuildManager(args, process_manager, environment_manager)
    conan_options = {'version': builder.compiler.version, 'libcxx': builder.stl.lib}
    if builder.is_msvc:
        conan_options = {'version': builder.compiler.version}

    env = BuildEnvironment(builder.use_conan, process_manager, environment_manager)
    env.prepare()

    if builder.use_conan:
        env.prepare_conan(conan_options)
        env.run_conan_install()

    builder.run_cmake()
    builder.build()
    builder.copy_files()


if __name__ == '__main__':
    main()

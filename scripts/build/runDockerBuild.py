import argparse
import sys
from pathlib import Path

from BasicBuildManager import BasicBuildManager
from environment import EnvironmentManager
from process import ProcessManager

CCACHE_ROOT = '/jenkins_cache/ccache'
CONAN_ROOT = '/jenkins_cache/conan'

SRC_DIR = Path('catapult-src').resolve()
OUTPUT_DIR = Path('output').resolve()
BINARIES_DIR = OUTPUT_DIR / 'binaries'


class OptionsManager(BasicBuildManager):
    def __init__(self, args):
        super().__init__(args.compiler_configuration, args.build_configuration)
        self.operating_system = args.operating_system

    @property
    def image_type(self):
        return 'release' if self.is_release else 'test'

    @property
    def version(self):
        version = self.image_type
        if self.enable_diagnostics:
            version += '-diagnostics'

        return '-'.join([version] + self.sanitizers + [self.architecture])

    @property
    def build_base_image_name(self):
        if self.use_conan:
            name_parts = [self.operating_system, self.versioned_compiler, 'conan']
        else:
            name_parts = [self.operating_system, self.compilation_friendly_name]

        return 'symbolplatform/symbol-server-build-base:{}'.format('-'.join(name_parts))

    @property
    def prepare_base_image_name(self):
        return 'symbolplatform/symbol-server-test-base:{}'.format(self.operating_system)

    @property
    def ccache_path(self):
        return Path(CCACHE_ROOT) / ('release' if self.is_release else 'all')

    @property
    def conan_path(self):
        return Path(CONAN_ROOT) / ('clang' if self.is_clang else 'gcc')

    def docker_run_settings(self):
        settings = [
            ('CC', self.compiler.c),
            ('CXX', self.compiler.cpp),
            ('CCACHE_DIR', '/ccache')
        ]

        return ['--env={}={}'.format(key, value) for key, value in sorted(settings)]


def get_volume_mappings(ccache_path, conan_path):
    mappings = [
        (SRC_DIR, '/catapult-src'),
        (BINARIES_DIR.resolve(), '/binaries'),
        (conan_path, '/conan'),
        (ccache_path, '/ccache')
    ]

    return ['--volume={}:{}'.format(str(key), value) for key, value in sorted(mappings)]


def create_docker_run_command(options, compiler_configuration_filepath, build_configuration_filepath, user):
    docker_run_settings = options.docker_run_settings()
    volume_mappings = get_volume_mappings(options.ccache_path, options.conan_path)

    docker_args = [
        'docker', 'run',
        '--rm',
        '--user={}'.format(user),
    ] + docker_run_settings + volume_mappings + [
        options.build_base_image_name,
        'python3', '/catapult-src/scripts/build/runDockerBuildInnerBuild.py',
        # assume paths are relative to workdir
        '--compiler-configuration=/{}'.format(compiler_configuration_filepath),
        '--build-configuration=/{}'.format(build_configuration_filepath)
    ]

    return docker_args


def cleanup_directories(environment_manager, ccache_root_directory, conan_root_directory):
    environment_manager.rmtree(OUTPUT_DIR)
    environment_manager.mkdirs(BINARIES_DIR)

    environment_manager.mkdirs(ccache_root_directory, exist_ok=True)
    environment_manager.mkdirs(conan_root_directory, exist_ok=True)


def prepare_docker_image(process_manager, container_id, prepare_replacements):
    destination_image_label = prepare_replacements['destination_image_label']
    cid_filepath = Path('{}.cid'.format(destination_image_label))
    if not container_id:
        if cid_filepath.exists():
            cid_filepath.unlink()

    build_disposition = prepare_replacements['build_disposition']
    disposition_to_repository_map = {
        'tests': 'symbol-server-test',
        'private': 'symbol-server-private',
        'public': 'symbol-server'
    }
    destination_repository = disposition_to_repository_map[build_disposition]

    destination_image_name = 'symbolplatform/{}:{}'.format(destination_repository, destination_image_label)
    process_manager.dispatch_subprocess([
        'docker', 'run',
        '--cidfile={}'.format(cid_filepath),
        '--volume={}:/scripts'.format(SRC_DIR / 'scripts' / 'build'),
        '--volume={}:/data'.format(OUTPUT_DIR),
        'registry.hub.docker.com/{}'.format(prepare_replacements['base_image_name']),
        'python3', '/scripts/runDockerBuildInnerPrepare.py',
        '--disposition={}'.format(build_disposition)
    ])

    if not container_id:
        with open(cid_filepath, 'rt') as cid_infile:
            container_id = cid_infile.read()

    process_manager.dispatch_subprocess(['docker', 'commit', container_id, destination_image_name])


def main():
    parser = argparse.ArgumentParser(description='catapult project build generator')
    parser.add_argument('--compiler-configuration', help='path to compiler configuration yaml', required=True)
    parser.add_argument('--build-configuration', help='path to build configuration yaml', required=True)
    parser.add_argument('--operating-system', help='operating system', required=True)
    parser.add_argument('--user', help='docker user', required=True)
    parser.add_argument('--destination-image-label', help='docker destination image label', required=True)
    parser.add_argument('--dry-run', help='outputs desired commands without runing them', action='store_true')
    parser.add_argument('--base-image-names-only', help='only output the base image names', action='store_true')
    args = parser.parse_args()

    options = OptionsManager(args)

    if args.base_image_names_only:
        print(options.build_base_image_name)
        print(options.prepare_base_image_name)
        return

    docker_run = create_docker_run_command(options, args.compiler_configuration, args.build_configuration, args.user)

    environment_manager = EnvironmentManager(args.dry_run)
    environment_manager.rmtree(OUTPUT_DIR)
    environment_manager.mkdirs(BINARIES_DIR)
    environment_manager.mkdirs(options.ccache_path / 'tmp', exist_ok=True)
    environment_manager.mkdirs(options.conan_path, exist_ok=True)

    print('building project')

    process_manager = ProcessManager(args.dry_run)

    return_code = process_manager.dispatch_subprocess(docker_run)
    if return_code:
        sys.exit(return_code)

    print('copying files')

    environment_manager.chdir(OUTPUT_DIR)

    for folder_name in ['scripts', 'seed', 'resources']:
        environment_manager.copy_tree_with_symlinks(SRC_DIR / folder_name, folder_name)

    environment_manager.chdir(SRC_DIR)

    print('building docker image')

    container_id = '<dry_run_container_id>' if args.dry_run else None
    prepare_docker_image(process_manager, container_id, {
        'base_image_name': options.prepare_base_image_name,
        'destination_image_label': args.destination_image_label,
        'build_disposition': options.build_disposition
    })


if __name__ == '__main__':
    main()

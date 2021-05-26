import argparse
import sys
from pathlib import Path

from environment import EnvironmentManager
from process import ProcessManager

MONGO_DIR = Path('mongo')
OUTPUT_DIR = Path('catapult-data')


def get_image_label(image_name):
    return image_name[image_name.index(':') + 1:]


def create_docker_compose_command(mode):
    return [
        'docker-compose', 'up',
        '--no-color',
        '--abort-on-container-exit',
        '--exit-code-from', mode
    ]


def prepare_docker_compose_file(input_filepath, prepare_replacements, outfile):
    image_name = prepare_replacements['image_name']
    replacements = [
        ('{{IMAGE_NAME}}', image_name),
        ('{{COMPILER_CONFIGURATION}}', prepare_replacements['compiler_configuration']),
        ('{{USER}}', '"{}"'.format(prepare_replacements['user'])),

        ('{{BUILD_NUMBER}}', get_image_label(image_name)),
        ('{{NETWORK_IP}}', '3000'),
        ('{{GTESTFILTER}}', '*'),
        ('{{STRESSCOUNT}}', '1'),
        ('{{VERBOSITY}}', prepare_replacements['verbosity'])
    ]

    with open(input_filepath, 'rt') as infile:
        contents = infile.read()
        for replacement in replacements:
            contents = contents.replace(replacement[0], replacement[1])

        outfile.write(contents)


def main():
    parser = argparse.ArgumentParser(description='catapult tests runner')
    parser.add_argument('--image', help='docker tests image', required=True)
    parser.add_argument('--compiler-configuration', help='path to compiler configuration yaml', required=True)
    parser.add_argument('--user', help='docker user', required=True)
    parser.add_argument('--mode', help='test mode', choices=('bench', 'test', 'lint'), required=True)
    parser.add_argument('--verbosity', help='verbosity level', default='max')
    parser.add_argument('--dry-run', help='outputs desired commands without runing them', action='store_true')
    args = parser.parse_args()

    process_manager = ProcessManager(args.dry_run)

    compose_template_directory = Path(__file__).parent / 'templates'
    compose_template_filepath = compose_template_directory / 'Run{}.yaml'.format(args.mode.capitalize())
    print('processing template from {}'.format(compose_template_filepath))
    prepare_replacements = {
        'image_name': args.image,
        'compiler_configuration': args.compiler_configuration,
        'user': args.user,
        'verbosity': args.verbosity
    }
    prepare_docker_compose_file(compose_template_filepath, prepare_replacements, sys.stdout)

    if not args.dry_run:
        with open('docker-compose.yaml', 'wt') as outfile:
            prepare_docker_compose_file(compose_template_filepath, prepare_replacements, outfile)

    environment_manager = EnvironmentManager(args.dry_run)
    environment_manager.set_env_var('COMPOSE_HTTP_TIMEOUT', '200')
    environment_manager.mkdirs(OUTPUT_DIR / 'logs', exist_ok=True)
    environment_manager.mkdirs(OUTPUT_DIR / 'workdir', exist_ok=True)

    if 'test' == args.mode:
        environment_manager.mkdirs(MONGO_DIR / get_image_label(args.image))

    docker_compose_args = create_docker_compose_command(args.mode)
    if process_manager.dispatch_subprocess(docker_compose_args, handle_error=False):
        print('tests failed')
        sys.exit(1)

    print('tests succeeded')


if __name__ == '__main__':
    main()

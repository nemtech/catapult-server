from configuration import load_build_configuration, load_compiler_configuration


class BasicBuildManager:
    def __init__(self, compiler_configuration_filepath, build_configuration_filepath):
        compiler_configuration = load_compiler_configuration(compiler_configuration_filepath)
        build_configuration = load_build_configuration(build_configuration_filepath)

        self.compiler = compiler_configuration.compiler
        self.stl = compiler_configuration.stl
        self.sanitizers = compiler_configuration.sanitizers
        self.architecture = compiler_configuration.architecture

        self.build_disposition = build_configuration.disposition
        self.use_conan = build_configuration.use_conan
        self.enable_diagnostics = build_configuration.enable_diagnostics

        if self.sanitizers and self.use_conan:
            raise RuntimeError('sanitizer cannot be used with conan-based builds')

    @property
    def is_clang(self):
        return self.compiler.c.startswith('clang')

    @property
    def is_msvc(self):
        return self.compiler.c.startswith('msvc')

    @property
    def is_release(self):
        return 'tests' != self.build_disposition

    @property
    def build_configuration(self):
        return 'RelWithDebInfo'

    @property
    def versioned_compiler(self):
        return '{}-{}'.format(self.compiler.c, self.compiler.version)

    @property
    def compilation_friendly_name(self):
        name_parts = [self.versioned_compiler] + self.sanitizers + [self.architecture]
        return '-'.join(name_parts)

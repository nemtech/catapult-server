pipeline {
    agent any

    parameters {
        gitParameter branchFilter: 'origin/(.*)', defaultValue: 'main', name: 'MANUAL_GIT_BRANCH', type: 'PT_BRANCH'
    }

    options {
        ansiColor('css')
        timestamps()
    }

    stages {
        stage('print env') {
            steps {
                echo """
                            env.GIT_BRANCH: ${env.GIT_BRANCH}
                         MANUAL_GIT_BRANCH: ${MANUAL_GIT_BRANCH}
                """
            }
        }

        stage('build servers') {
            parallel {
                stage('gcc 10 (conan)') {
                    steps {
                        script {
                            dispatch_build_job('gcc-10', 'tests-conan')
                        }
                    }
                }
                stage('gcc 10 (metal)') {
                    steps {
                        script {
                            dispatch_build_job('gcc-10', 'tests-metal')
                        }
                    }
                }

                stage('clang 12 (conan)') {
                    steps {
                        script {
                            dispatch_build_job('clang-12', 'tests-conan')
                        }
                    }
                }
                stage('clang 12 (metal)') {
                    steps {
                        script {
                            dispatch_build_job('clang-12', 'tests-metal')
                        }
                    }
                }

                stage('clang ausan') {
                    steps {
                        script {
                            dispatch_build_job('clang-ausan', 'tests-metal')
                        }
                    }
                }
                stage('clang tsan') {
                    steps {
                        script {
                            dispatch_build_job('clang-tsan', 'tests-metal')
                        }
                    }
                }
                stage('clang diagnostics') {
                    steps {
                        script {
                            dispatch_build_job('clang-12', 'tests-diagnostics')
                        }
                    }
                }
            }
        }
    }
}

def dispatch_build_job(compiler_configuration, build_configuration) {
    build job: 'server-pipelines/catapult-server-build-catapult-project', parameters: [
        string(name: 'COMPILER_CONFIGURATION', value: "${compiler_configuration}"),
        string(name: 'BUILD_CONFIGURATION', value: "${build_configuration}"),
        string(name: 'OPERATING_SYSTEM', value: 'ubuntu'),
        string(name: 'MANUAL_GIT_BRANCH', value: "${params.MANUAL_GIT_BRANCH}")
    ]
}

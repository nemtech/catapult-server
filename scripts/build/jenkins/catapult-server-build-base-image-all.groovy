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

        stage('build base images') {
            parallel {
                stage('gcc 10') {
                    steps {
                        script {
                            dispatch_build_base_image_job('gcc-10', 'ubuntu', true)
                        }
                    }
                }
                stage('gcc 8 [debian]') {
                    steps {
                        script {
                            dispatch_build_base_image_job('gcc-8', 'debian', false)
                        }
                    }
                }
                stage('gcc 10 westmere') {
                    steps {
                        script {
                            dispatch_build_base_image_job('gcc-10-westmere', 'ubuntu', true)
                        }
                    }
                }
                stage('gcc 11 [fedora]') {
                    steps {
                        script {
                            dispatch_build_base_image_job('gcc-11', 'fedora', false)
                        }
                    }
                }

                stage('clang 11') {
                    steps {
                        script {
                            dispatch_build_base_image_job('clang-11', 'ubuntu', false)
                        }
                    }
                }
                stage('clang 12') {
                    steps {
                        script {
                            dispatch_build_base_image_job('clang-12', 'ubuntu', true)
                        }
                    }
                }

                stage('clang ausan') {
                    steps {
                        script {
                            dispatch_build_base_image_job('clang-ausan', 'ubuntu', false)
                        }
                    }
                }
                stage('clang tsan') {
                    steps {
                        script {
                            dispatch_build_base_image_job('clang-tsan', 'ubuntu', false)
                        }
                    }
                }

                stage('release base image') {
                    steps {
                        script {
                            dispatch_prepare_base_image_job('release', 'ubuntu')
                        }
                    }
                }

                stage('test base image') {
                    steps {
                        script {
                            dispatch_prepare_base_image_job('test', 'ubuntu')
                        }
                    }
                }
                stage('test base image [debian]') {
                    steps {
                        script {
                            dispatch_prepare_base_image_job('test', 'debian')
                        }
                    }
                }
                stage('test base image [fedora]') {
                    steps {
                        script {
                            dispatch_prepare_base_image_job('test', 'fedora')
                        }
                    }
                }
            }
        }
    }
}

def dispatch_build_base_image_job(compiler_configuration, operating_system, should_build_conan_layer) {
    build job: 'server-pipelines/catapult-server-build-base-image', parameters: [
        string(name: 'COMPILER_CONFIGURATION', value: "${compiler_configuration}"),
        string(name: 'OPERATING_SYSTEM', value: "${operating_system}"),
        string(name: 'SHOULD_BUILD_CONAN_LAYER', value: "${should_build_conan_layer}"),
        string(name: 'MANUAL_GIT_BRANCH', value: "${params.MANUAL_GIT_BRANCH}")
    ]
}

def dispatch_prepare_base_image_job(image_type, operating_system) {
    build job: 'server-pipelines/catapult-server-prepare-base-image', parameters: [
        string(name: 'IMAGE_TYPE', value: "${image_type}"),
        string(name: 'OPERATING_SYSTEM', value: "${operating_system}"),
        string(name: 'MANUAL_GIT_BRANCH', value: "${params.MANUAL_GIT_BRANCH}")
    ]
}


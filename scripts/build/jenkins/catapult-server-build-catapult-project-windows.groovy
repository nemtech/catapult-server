pipeline {
    agent {
        label 'windows-8cores-16gig'
    }

    parameters {
        gitParameter branchFilter: 'origin/(.*)', defaultValue: "${env.GIT_BRANCH}", name: 'MANUAL_GIT_BRANCH', type: 'PT_BRANCH'
        choice name: 'COMPILER_CONFIGURATION',
            choices: ['msvc-2019'],
            description: 'compiler configuration'
        choice name: 'BUILD_CONFIGURATION',
            choices: ['tests-conan'],
            description: 'build configuration'
        choice name: 'OPERATING_SYSTEM',
            choices: ['windows'],
            description: 'operating system'

        string name: 'TEST_IMAGE_LABEL', description: 'docker test image label', defaultValue: ''
        choice name: 'TEST_MODE',
            choices: ['test', 'bench', 'none'],
            description: 'test mode'
        choice name: 'TEST_VERBOSITY',
            choices: ['suite', 'test', 'max'],
            description: 'output verbosity level'

        booleanParam name: 'SHOULD_PUBLISH_BUILD_IMAGE', description: 'true to publish build image', defaultValue: false
    }

    environment {
        DOCKER_URL = 'https://registry.hub.docker.com'
        DOCKER_CREDENTIALS_ID = 'docker-hub-token-symbolserverbot'
    }

    options {
        ansiColor('css')
        timestamps()
    }

    stages {
        stage('prepare') {
            stages {
                stage('prepare variables') {
                    steps {
                        script {
                            fully_qualified_user = sh(
                                script: 'echo "$(id -u):$(id -g)"',
                                returnStdout: true
                            ).trim()

                            build_image_label = '' != TEST_IMAGE_LABEL ? TEST_IMAGE_LABEL : get_build_image_label()
                            build_image_full_name = "symbolplatform/symbol-server-test:${build_image_label}"
                        }
                    }
                }
                stage('print env') {
                    steps {
                        echo """
                                    env.GIT_BRANCH: ${env.GIT_BRANCH}
                                 MANUAL_GIT_BRANCH: ${MANUAL_GIT_BRANCH}

                            COMPILER_CONFIGURATION: ${COMPILER_CONFIGURATION}
                               BUILD_CONFIGURATION: ${BUILD_CONFIGURATION}
                                  OPERATING_SYSTEM: ${OPERATING_SYSTEM}

                                  TEST_IMAGE_LABEL: ${TEST_IMAGE_LABEL}
                                         TEST_MODE: ${TEST_MODE}
                                    TEST_VERBOSITY: ${TEST_VERBOSITY}

                        SHOULD_PUBLISH_BUILD_IMAGE: ${SHOULD_PUBLISH_BUILD_IMAGE}

                              fully_qualified_user: ${fully_qualified_user}
                                 build_image_label: ${build_image_label}
                             build_image_full_name: ${build_image_full_name}
                        """
                    }
                }
                stage('git checkout') {
                    when {
                        expression { is_manual_build() }
                    }
                    steps {
                        dir('catapult-src') {
                            git branch: "${get_branch_name()}",
                                url: 'https://github.com/nemtech/catapult-server.git'
                        }
                    }
                }
            }
        }
        stage('build') {
            when {
                expression { is_build_enabled() }
            }
            stages {
                stage('prepare variables') {
                    steps {
                        script {
                            bat 'set'
                            sh 'docker run --rm hello-world'
                            run_docker_build_command = """
                                python catapult-src/scripts/build/runDockerBuild.py \
                                    --compiler-configuration catapult-src/scripts/build/configurations/${COMPILER_CONFIGURATION}.yaml \
                                    --build-configuration catapult-src/scripts/build/configurations/${BUILD_CONFIGURATION}.yaml \
                                    --operating-system ${OPERATING_SYSTEM} \
                                    --user ${fully_qualified_user} \
                                    --destination-image-label ${build_image_label} \
                            """
                        }
                    }
                }
                stage('pull dependency images') {
                    steps {
                        script {
                            base_image_names = sh(
                                script: "${run_docker_build_command} --base-image-names-only",
                                returnStdout: true
                            ).split('\n')

                            docker.withRegistry(DOCKER_URL, DOCKER_CREDENTIALS_ID) {
                                for (base_image_name in base_image_names)
                                    docker.image(base_image_name).pull()
                            }
                        }
                    }
                }
                // stage('lint') {
                //     steps {
                //         withEnv(['PATH+PS=C:\\ProgramData\\chocolatey\\lib\\docker-compose\\tools']) {
                //             sh """
                //                 python catapult-src/scripts/build/runDockerTests.py \
                //                     --image registry.hub.docker.com/symbolplatform/symbol-server-test-base:${OPERATING_SYSTEM} \
                //                     --compiler-configuration catapult-src/scripts/build/configurations/${COMPILER_CONFIGURATION}.yaml \
                //                     --user ${fully_qualified_user} \
                //                     --mode lint
                //             """
                //         }
                //     }
                // }
                stage('build') {
                    steps {
                        sh "${run_docker_build_command}"
                    }
                }
                stage('push built image') {
                    when {
                        expression { SHOULD_PUBLISH_BUILD_IMAGE.toBoolean() }
                    }
                    steps {
                        script {
                            docker.withRegistry(DOCKER_URL, DOCKER_CREDENTIALS_ID) {
                                docker.image(build_image_full_name).push()
                            }
                        }
                    }
                }
            }
            post {
                always {
                    recordIssues enabledForFailure: true, tool: pyLint(pattern: 'catapult-data/logs/pylint.log')
                    recordIssues enabledForFailure: true, tool: pep8(pattern: 'catapult-data/logs/pycodestyle.log')
                    recordIssues enabledForFailure: true, tool: gcc(pattern: 'catapult-data/logs/isort.log', name: 'isort', id: 'isort')

                    recordIssues enabledForFailure: true,
                        tool: gcc(pattern: 'catapult-data/logs/shellcheck.log', name: 'shellcheck', id: 'shellcheck')
                }
            }
        }
        stage('test') {
            when {
                expression { is_test_enabled() }
            }
            stages {
                stage('pull dependency images') {
                    when {
                        expression { is_custom_test_image() }
                    }
                    steps {
                        script {
                            docker.withRegistry(DOCKER_URL, DOCKER_CREDENTIALS_ID) {
                                docker.image(build_image_full_name).pull()
                            }
                        }
                    }
                }
                stage('run tests') {
                    steps {
                        script {
                            if (is_custom_test_image())
                                test_image_name = "registry.hub.docker.com/symbolplatform/symbol-server-test:${build_image_label}"
                            else
                                test_image_name = "symbolplatform/symbol-server-test:${build_image_label}"

                            sh """
                                python catapult-src/scripts/build/runDockerTests.py \
                                    --image ${test_image_name} \
                                    --compiler-configuration catapult-src/scripts/build/configurations/${COMPILER_CONFIGURATION}.yaml \
                                    --user ${fully_qualified_user} \
                                    --mode ${TEST_MODE} \
                                    --verbosity ${TEST_VERBOSITY}
                            """
                        }
                    }
                }
            }
        }
    }
    post {
        always {
            junit 'catapult-data/logs/*.xml'

            dir('catapult-data') {
                deleteDir()
            }
            dir('mongo') {
                deleteDir()
            }
        }
    }
}

def is_build_enabled() {
    return 'none' != BUILD_CONFIGURATION
}

def is_test_enabled() {
    return 'none' != TEST_MODE
}

def is_manual_build() {
    return null != MANUAL_GIT_BRANCH && '' != MANUAL_GIT_BRANCH && 'null' != MANUAL_GIT_BRANCH
}

def is_custom_test_image() {
    return '' != TEST_IMAGE_LABEL
}

def get_branch_name() {
    return is_manual_build() ? MANUAL_GIT_BRANCH : env.GIT_BRANCH
}

def get_build_image_label() {
    friendly_branch_name = get_branch_name()
    if (0 == friendly_branch_name.indexOf('origin/'))
        friendly_branch_name = friendly_branch_name.substring(7)

    friendly_branch_name = friendly_branch_name.replaceAll('/', '-')
    return "catapult-server-${friendly_branch_name}-${env.BUILD_NUMBER}"
}

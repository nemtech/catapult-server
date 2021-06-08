pipeline {
    agent {
        label 'windows-8cores-16gig'
    }

    parameters {
        gitParameter branchFilter: 'origin/(.*)', defaultValue: 'main', name: 'MANUAL_GIT_BRANCH', type: 'PT_BRANCH'
        choice name: 'COMPILER_CONFIGURATION',
            choices: ['msvc'],
            description: 'compiler configuration'
        choice name: 'OPERATING_SYSTEM',
            choices: ['windows'],
            description: 'operating system'

        booleanParam name: 'SHOULD_BUILD_CONAN_LAYER', description: 'true to build conan layer', defaultValue: false
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
                            dest_image_name = "symbolplatform/symbol-server-build-base:${OPERATING_SYSTEM}-${COMPILER_CONFIGURATION}"

                            base_image_dockerfile_generator_command = """
                                python3 ./scripts/build/baseImageDockerfileGenerator.py \
                                    --compiler-configuration scripts/build/configurations/${COMPILER_CONFIGURATION}.yaml \
                                    --operating-system ${OPERATING_SYSTEM} \
                                    --versions ./scripts/build/versions.properties \
                            """
                        }
                    }
                }
                stage('print env') {
                    steps {
                        echo """
                                    env.GIT_BRANCH: ${env.GIT_BRANCH}
                                 MANUAL_GIT_BRANCH: ${MANUAL_GIT_BRANCH}

                            COMPILER_CONFIGURATION: ${COMPILER_CONFIGURATION}
                                  OPERATING_SYSTEM: ${OPERATING_SYSTEM}
                          SHOULD_BUILD_CONAN_LAYER: ${SHOULD_BUILD_CONAN_LAYER}

                                   dest_image_name: ${dest_image_name}
                        """
                    }
                }
            }
        }
        stage('build image') {
            stages {
                stage('compiler image') {
                    steps {
                        script {
                            dir ('scripts/build/compilers/windows-msvc-2019') {
                                image_name = 'symbolplatform/symbol-server-build-base:windows-msvc-16-conan'
                                docker.build(image_name)

                                docker.withRegistry(DOCKER_URL, DOCKER_CREDENTIALS_ID) {
                                    docker.image(image_name).push()
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

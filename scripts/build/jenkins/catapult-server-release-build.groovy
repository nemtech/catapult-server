pipeline {
    agent {
        label 'ubuntu-20.04-8cores-16Gig'
    }

    parameters {
        gitParameter branchFilter: 'origin/(.*)', defaultValue: "${env.GIT_BRANCH}", name: 'MANUAL_GIT_BRANCH', type: 'PT_BRANCH'
        choice name: 'COMPILER_CONFIGURATION',
            choices: ['gcc-10', 'gcc-10-westmere', 'gcc-11', 'clang-11', 'clang-12'],
            description: 'compiler configuration'
        choice name: 'BUILD_CONFIGURATION',
            choices: ['release-private', 'release-public'],
            description: 'build configuration'
        choice name: 'OPERATING_SYSTEM',
            choices: ['ubuntu', 'fedora'],
            description: 'operating system'

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
        // stage('preliminary') {
        //     when {
        //         expression { is_public_build() && SHOULD_PUBLISH_BUILD_IMAGE.toBoolean() }
        //     }

        //     steps {
        //         script {
        //             timeout(time: 10, unit: 'SECONDS') {
        //                 input(
        //                     id: "userInput",
        //                     message: "Are you sure you want to create public build?"
        //                 )
        //             }
        //         }
        //     }
        // }
        stage('prepare') {
            stages {
                stage('prepare variables') {
                    steps {
                        script {
                            fully_qualified_user = sh(
                                script: 'echo "$(id -u):$(id -g)"',
                                returnStdout: true
                            ).trim()

                            build_image_repo = get_build_image_repo()
                            build_image_label = get_build_image_label()
                            build_image_full_name = "symbolplatform/${build_image_repo}:${build_image_label}"
                        }
                    }
                }
                stage('print env') {
                    steps {
                        echo """
                                    env.GIT_BRANCH: ${env.GIT_BRANCH}
                                    env.GIT_COMMIT: ${env.GIT_COMMIT}
                                 MANUAL_GIT_BRANCH: ${MANUAL_GIT_BRANCH}

                            COMPILER_CONFIGURATION: ${COMPILER_CONFIGURATION}
                               BUILD_CONFIGURATION: ${BUILD_CONFIGURATION}
                                  OPERATING_SYSTEM: ${OPERATING_SYSTEM}

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
                                url: 'https://github.com/nemgrouplimited/catapult-server.git',
								credentialsId: '24750a46-613b-466a-8b3b-c61efd2fd473'
                        }
                    }
                }
            }
        }
        stage('build') {
            stages {
                stage('prepare variables') {
                    steps {
                        script {
                            run_docker_build_command = """
                                python3 catapult-src/scripts/build/runDockerBuild.py \
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
                            short_label = get_short_image_label()
                            docker.withRegistry(DOCKER_URL, DOCKER_CREDENTIALS_ID) {
                                built_image = docker.image(build_image_full_name)
                                built_image.push()
                                built_image.push("${short_label}")
                            }
                        }
                    }
                }
            }
        }

    //     stage('bump version') {
    //         when {
    //             expression { is_public_build() && SHOULD_PUBLISH_BUILD_IMAGE.toBoolean() }
    //         }
    //         steps {
    //             script {
    //                 sh 'ls -alh ./catapult-src/scripts/build/versions.properties'
    //                 new_version = bump_version()

    //                 dir('catapult-src') {
    //                     // NOTE: assuming correct branch is checked out
    //                     withCredentials([usernamePassword(
    //                             credentialsId: 'nemtechopsbot-git',
    //                             passwordVariable: 'GIT_PASSWORD',
    //                             usernameVariable: 'GIT_USERNAME')]) {
    //                         sh """
    //                             git config --global user.email "nemtechopsbot@127.0.0.1"
    //                             git config --global user.name "nemtechopsbot"

    //                             git add ./scripts/build/server.version.yaml
    //                             git add ./src/catapult/version/version_inc.h
    //                             git commit -m \"bump version to ${bumped_version}\"
    //                             git push https://${GIT_USERNAME}:${GIT_PASSWORD}@github.com/nemtech/catapult-server.git
    //                         """
    //                     }

    //                     sh """
    //                         git status
    //                         git log -1
    //                     """
    //                 }
    //             }
    //         }
    //     }
    // }
}

def is_public_build() {
    return 'release-public' == BUILD_CONFIGURATION
}

def is_manual_build() {
    return null != MANUAL_GIT_BRANCH && '' != MANUAL_GIT_BRANCH && 'null' != MANUAL_GIT_BRANCH
}

def get_branch_name() {
    return is_manual_build() ? MANUAL_GIT_BRANCH : env.GIT_BRANCH
}

def bump_version() {
    version_path = './catapult-src/scripts/build/server.version.yaml'
    data = readYaml(file: version_path)
    version = data.version

    def (major, minor, patch, build) = version.tokenize('.').collect { it.toInteger() }
    def bumped_version = "${major}.${minor}.${patch}.${build + 1}"

    data.version = bumped_version
    writeYaml(file: version_path, data: data, overwrite: true)

    versioninc_contents = readFile(file: './catapult-src/scripts/build/templates/version_inc.h')
    versioninc_contents = versioninc_contents
        .replaceAll('\\{\\{MAJOR\\}\\}', "${major}")
        .replaceAll('\\{\\{MINOR\\}\\}', "${minor}")
        .replaceAll('\\{\\{REVISION\\}\\}', "${patch}")
        .replaceAll('\\{\\{BUILD\\}\\}', "${build}")
    writeFile(file: './catapult-src/src/catapult/version/version_inc.h', text: versioninc_contents)

    return bumped_version
}

def get_public_version() {
    version_path = './catapult-src/scripts/build/server.version.yaml'
    data = readYaml(file: version_path)
    return data.version
}

def get_build_image_repo() {
    return is_public_build() ? 'symbol-server' : 'symbol-server-private'
}

def get_architecture_label() {
    data = readYaml(file: "./catapult-src/scripts/build/configurations/${COMPILER_CONFIGURATION}.yaml")
    architecture = data.architecture

    if ('skylake' == architecture)
        return ''

    return "-${architecture}"
}

def get_build_image_label() {
    friendly_branch_name = get_branch_name()
    if (0 == friendly_branch_name.indexOf('origin/'))
        friendly_branch_name = friendly_branch_name.substring(7)

    friendly_branch_name = friendly_branch_name.replaceAll('/', '-')
    architecture = get_architecture_label()
    git_hash="${env.GIT_COMMIT}".substring(0, 8)
    return "${COMPILER_CONFIGURATION}-${friendly_branch_name}${architecture}-${git_hash}"
}

def get_short_image_label() {
    architecture = get_architecture_label()
    if (!is_public_build())
        return "${COMPILER_CONFIGURATION}${architecture}"

    version_string = get_public_version()
    return "${COMPILER_CONFIGURATION}-${version_string}${architecture}"
}

pipeline {
    agent none

    parameters {
        gitParameter branchFilter: 'origin/(.*)', defaultValue: 'main', name: 'MANUAL_GIT_BRANCH', type: 'PT_BRANCH'
        choice name: 'OPERATING_SYSTEM',
            choices: ['ubuntu', 'fedora', 'debian', 'windows'],
            description: 'operating system'
        choice name: 'IMAGE_TYPE',
            choices: ['release', 'test'],
            description: 'image type'

        choice name: 'AGENT_LABEL',
            choices: ['ubuntu-20.04-8cores-16Gig', 'windows-8cores-16gig'],
            description: 'agent label'
    }

    options {
        ansiColor('css')
        timestamps()
    }

    environment {
        DOCKER_URL = 'https://registry.hub.docker.com'
        DOCKER_CREDENTIALS_ID = 'docker-hub-token-symbolserverbot'
    }

    stages {
        stage('run build on single agent') {
            agent {
                label "${params.AGENT_LABEL}"
            }

            stages {
                stage('print env') {
                    steps {
                        echo """
                                    env.GIT_BRANCH: ${env.GIT_BRANCH}
                                MANUAL_GIT_BRANCH: ${MANUAL_GIT_BRANCH}

                                OPERATING_SYSTEM: ${OPERATING_SYSTEM}
                                        IMAGE_TYPE: ${IMAGE_TYPE}
                        """
                    }
                }
                stage('prepare Dockerfile') {
                    steps {
                        script {
                            properties = readProperties(file: './scripts/build/versions.properties')
                            base_image_name = properties[params.OPERATING_SYSTEM]

                            dockerfile_template = "./scripts/build/templates/${params.OPERATING_SYSTEM.capitalize()}${params.IMAGE_TYPE.capitalize()}BaseImage.Dockerfile"
                            dockerfile_contents = readFile(file: dockerfile_template)
                            dockerfile_contents = dockerfile_contents.replaceAll('\\{\\{BASE_IMAGE\\}\\}', "${base_image_name}")

                            writeFile(file: 'Dockerfile', text: dockerfile_contents)
                        }
                    }
                }
                stage('print Dockerfile') {
                    steps {
                        sh '''
                            echo '*** Dockerfile ***'
                            cat Dockerfile
                        '''
                    }
                }

                stage('build image') {
                    steps {
                        script {
                            docker_image = docker.build "symbolplatform/symbol-server-${params.IMAGE_TYPE}-base:${params.OPERATING_SYSTEM}"
                            docker.withRegistry(DOCKER_URL, DOCKER_CREDENTIALS_ID) {
                                docker_image.push()
                            }
                        }
                    }
                }
            }
        }
    }
}

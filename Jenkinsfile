pipeline {
    agent {
        node { label "master" }
    }
    options {
        // TODO: set max. no. of concurrent builds to 2
        timeout(time: 8, unit: "HOURS")
        timestamps()
    }
    parameters {
        choice(name: "CHANNEL", choices: ["nightly", "dev", "beta", "release"], description: "")
        booleanParam(name: "WIPE_WORKSPACE", defaultValue: false, description: "")
        booleanParam(name: "RUN_INIT", defaultValue: false, description: "")
        booleanParam(name: "DISABLE_SCCACHE", defaultValue: false, description: "")
        // TODO: add SKIP_SIGNING
        booleanParam(name: "DEBUG", defaultValue: false, description: "")
    }
    environment {
        GITHUB_API = "https://api.github.com/repos/brave"
        GITHUB_CREDENTIAL_ID = "brave-builds-github-token-for-pr-builder"
        GITHUB_AUTH = credentials("${GITHUB_CREDENTIAL_ID}")
        BB_REPO = "https://${GITHUB_AUTH}@github.com/brave/brave-browser"
    }
    stages {
        stage("env") {
            steps {
                script {
                    CHANNEL = params.CHANNEL
                    WIPE_WORKSPACE = params.WIPE_WORKSPACE
                    RUN_INIT = params.RUN_INIT
                    DISABLE_SCCACHE = params.DISABLE_SCCACHE
                    DEBUG = params.DEBUG
                    BRANCH_TO_BUILD = (env.CHANGE_BRANCH.equals(null) ? env.BRANCH_NAME : env.CHANGE_BRANCH)
                    TARGET_BRANCH = (env.CHANGE_TARGET.equals(null) ? BRANCH_TO_BUILD : env.CHANGE_TARGET)
                    BRANCH_EXISTS_IN_BB = httpRequest(url: GITHUB_API + "/brave-browser/branches/" + BRANCH_TO_BUILD, validResponseCodes: "100:499", authentication: GITHUB_CREDENTIAL_ID, quiet: !DEBUG).status.equals(200)
                    if (env.CHANGE_BRANCH) {
                        def prNumber = readJSON(text: httpRequest(url: GITHUB_API + "/brave-core/pulls?head=brave:" + BRANCH_TO_BUILD, authentication: GITHUB_CREDENTIAL_ID, quiet: !DEBUG).content)[0].number
                        def prDetails = readJSON(text: httpRequest(url: GITHUB_API + "/brave-core/pulls/" + prNumber, authentication: GITHUB_CREDENTIAL_ID, quiet: !DEBUG).content)
                        env.SKIP = prDetails.mergeable_state.equals("draft") or prDetails.labels.count { label -> label.name.equals("CI/Skip") }.equals(1)
                    }
                }
            }
        }
        stage("abort") {
            steps {
                script {
                    if (env.SKIP == true) {
                        print "Aborting build as PR is in draft or has \"CI/Skip\" label."
                        stopCurrentBuild()
                    }
                    for (build in getBuilds()) {
                        if (build.isBuilding() && build.getNumber() < env.BUILD_NUMBER.toInteger()) {
                            print "Aborting older running build " + build
                            build.doStop()
                            // build.finish(hudson.model.Result.ABORTED, new java.io.IOException("Aborting build"))
                        }
                    }
                }
            }
        }
        stage("checkout") {
            when {
                expression { env.SKIP != true }
            }
            steps {
                sh """
                    set -e

                    if [ -d brave-browser ]; then
                        rm -rf brave-browser
                    fi

                    git clone --branch ${TARGET_BRANCH} ${BB_REPO} || git clone ${BB_REPO}

                    cd brave-browser
                    git config user.name brave-builds
                    git config user.email devops@brave.com
                    git config push.default simple
                """
            }
        }
        stage("branch-create") {
            when {
                allOf {
                    expression { env.SKIP != true }
                    expression { !BRANCH_EXISTS_IN_BB }
                }
            }
            steps {
                sh """
                    set -e

                    cd brave-browser
                    git checkout -b ${BRANCH_TO_BUILD}

                    echo "Pushing branch ..."
                    git push ${BB_REPO}
                """
            }
        }
        stage("branch-rebase") {
            when {
                allOf {
                    expression { env.SKIP != true }
                    expression { BRANCH_EXISTS_IN_BB }
                }
            }
            steps {
                sh """
                    set -e

                    cd brave-browser
                    git checkout ${BRANCH_TO_BUILD}

                    if [ "`cat package.json | jq -r .version`" != "`cat ../package.json | jq -r .version`" ]; then
                        set +e

                        echo "Version mismatch between brave-browser and brave-core in package.json! Attempting rebase on brave-browser..."

                        echo "Fetching latest changes and pruning refs..."
                        git fetch --prune

                        echo "Rebasing ${BRANCH_TO_BUILD} branch on brave-browser against ${TARGET_BRANCH}..."
                        git rebase origin/${TARGET_BRANCH}

                        if [ \$? -ne 0 ]; then
                            echo "Failed to rebase (conflicts), will need to be manually rebased!"
                            git rebase --abort
                        else
                            echo "Rebased, force pushing to brave-browser..."
                            git push --force ${BB_REPO}
                        fi

                        if [ "`cat package.json | jq -r .version`" != "`cat ../package.json | jq -r .version`" ]; then
                            echo "Version mismatch between brave-browser and brave-core in package.json! Please try rebasing this branch in brave-core as well."
                            exit 1
                        fi

                        set -e
                    fi
                """
            }
        }
        stage("build") {
            when {
                expression { env.SKIP != true }
            }
            steps {
                script {
                    try {
                        startBraveBrowserBuild()
                    }
                    catch (hudson.AbortException ex) {
                        print "Sleeping 6m so new branch is discovered or associated PR created in brave-browser..."
                        sleep(time: 6, unit: "MINUTES")
                        startBraveBrowserBuild()
                    }
                }
            }
        }
    }
}

@NonCPS
def stopCurrentBuild() {
    Jenkins.instance.getItemByFullName(env.JOB_NAME).getLastBuild().doStop()
}

@NonCPS
def getBuilds() {
    return Jenkins.instance.getItemByFullName(env.JOB_NAME).builds
}

def startBraveBrowserBuild() {
    def prDetails = readJSON(text: httpRequest(url: GITHUB_API + "/brave-browser/pulls?head=brave:" + BRANCH_TO_BUILD, authentication: GITHUB_CREDENTIAL_ID, quiet: !DEBUG).content)[0]
    def prNumber = prDetails ? prDetails.number : ""
    def refToBuild = prNumber ? "PR-" + prNumber : URLEncoder.encode(BRANCH_TO_BUILD, "UTF-8")
    params = [
        string(name: "CHANNEL", value: CHANNEL),
        booleanParam(name: "WIPE_WORKSPACE", value: WIPE_WORKSPACE),
        booleanParam(name: "RUN_INIT", value: RUN_INIT),
        booleanParam(name: "DISABLE_SCCACHE", value: DISABLE_SCCACHE),
        // TODO: add SKIP_SIGNING
        booleanParam(name: "DEBUG", value: DEBUG)
    ]
    currentBuild.result = build(job: "brave-browser-build-pr/" + refToBuild, parameters: params, propagate: false).result
}

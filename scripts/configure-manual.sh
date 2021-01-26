#!/bin/bash
# This program automates common/essential tasks for building catapult-server

function help {
	echo "$0 creates a _build directory ready to be compiled."
	echo "Available commands:"
	echo "    $prog install system_reqs          Installs apt dependencies. Requires sudo."
	echo "          debian packages: $debs"
	echo "    $prog download deps                Obtain 3rd party libs."
	echo "    $prog install deps                 Compile & install 3rd party libs."
	echo "    $prog                              Compile catapult-server."
	if [ "_$CAT_DEPS_DIR" == "_" ]; then
		echo "Environment variable CAT_DEPS_DIR not set. Required for storing source dependencies."
		echo "Default value is: $HOME/cat_deps_dir"
	else
		echo "Dependencies env.var CAT_DEPS_DIR is set to $CAT_DEPS_DIR"
	fi
}

function set_depsdir {
	depsdir=$CAT_DEPS_DIR
	boost_output_dir=$depsdir/boost
	if [ "_$CAT_DEPS_DIR" == "_" ]; then
		CAT_DEPS_DIR="$HOME/cat_deps_dir"
		depsdir=$CAT_DEPS_DIR
		echo "CAT_DEPS_DIR not found in env. Using default: $CAT_DEPS_DIR."
		warn_env=1
	fi
}

function exitok {
	if [ $warn_env -eq 1 ]; then
cat << EOF
Please export the environment variable CAT_DEPS_DIR

  export CAT_DEPS_DIR=$depsdir

Note: If you want the CAT_DEPS_DIR environment variable to persist across sessions make sure to include the last line in the ~/.profile or ~/.bashrc files.
EOF
	fi
	exit 0
}

function reqroot {
	if [ "_`whoami`" != "_root" ]; then
		echo "Please run as root. (or use sudo)"
		exit 1
	fi
}

function download_boost {
	local boost_ver=1_${1}_0
	local boost_ver_dotted=1.${1}.0

	curl -o boost_${boost_ver}.tar.gz -SL https://dl.bintray.com/boostorg/release/${boost_ver_dotted}/source/boost_${boost_ver}.tar.gz
	tar -xzf boost_${boost_ver}.tar.gz
	mv boost_${boost_ver} boost
}

function download_git_dependency {
	git clone git://github.com/${1}/${2}.git
	cd ${2}
	git checkout ${3}
	cd ..
}

function download_all {
	download_boost 75

	download_git_dependency google googletest release-1.10.0
	download_git_dependency google benchmark v1.5.2

	download_git_dependency mongodb mongo-c-driver 1.17.2
	download_git_dependency mongodb mongo-cxx-driver r3.6.1

	download_git_dependency zeromq libzmq v4.3.3
	download_git_dependency zeromq cppzmq v4.7.1

	download_git_dependency facebook rocksdb v6.13.3
}

function install_boost {
	cd boost
	./bootstrap.sh with-toolset=clang --prefix=${boost_output_dir}

	b2_options=()
	b2_options+=(--prefix=${boost_output_dir})
	./b2 ${b2_options[@]} -j $jobs stage release
	./b2 install ${b2_options[@]}
}

function install_git_dependency {
	cd ${2}
	mkdir _build
	cd _build

	cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX="$depsdir/${1}" ${cmake_options[@]} ..
	make -j $jobs && make install
}

function install_google_test {
	cmake_options=()
	cmake_options+=(-DCMAKE_POSITION_INDEPENDENT_CODE=ON)
	install_git_dependency google googletest
}

function install_google_benchmark {
	cmake_options=()
	cmake_options+=(-DBENCHMARK_ENABLE_GTEST_TESTS=OFF)
	install_git_dependency google benchmark
}

function install_mongo_c_driver {
	cmake_options=()
	cmake_options+=(-DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF)
	cmake_options+=(-DENABLE_MONGODB_AWS_AUTH=OFF)
	cmake_options+=(-DENABLE_TESTS=OFF)
	cmake_options+=(-DENABLE_EXAMPLES=OFF)
	cmake_options+=(-DENABLE_SASL=OFF)
	install_git_dependency mongodb mongo-c-driver
}

function install_mongo_cxx_driver {
	cmake_options=()
	cmake_options+=(-DBOOST_ROOT=${boost_output_dir})
	cmake_options+=(-DCMAKE_CXX_STANDARD=17)
	install_git_dependency mongodb mongo-cxx-driver
}

function install_zmq_lib {
	cmake_options=()
	cmake_options+=(-DWITH_TLS=OFF)
	install_git_dependency zeromq libzmq
}

function install_zmq_cpp {
	cmake_options=()
	cmake_options+=(-DCPPZMQ_BUILD_TESTS=OFF)
	install_git_dependency zeromq cppzmq
}

function install_rocks {
	cmake_options=()
	cmake_options+=(-DPORTABLE=1)
	cmake_options+=(-DWITH_TESTS=OFF)
	cmake_options+=(-DWITH_TOOLS=OFF)
	cmake_options+=(-DWITH_BENCHMARK_TOOLS=OFF)
	cmake_options+=(-DWITH_CORE_TOOLS=OFF)
	cmake_options+=(-DWITH_GFLAGS=OFF)
	install_git_dependency facebook rocksdb
}

function install_all {
	declare -a installers=(
		install_boost
		install_google_test
		install_google_benchmark
		install_mongo_c_driver
		install_mongo_cxx_driver
		install_zmq_lib
		install_zmq_cpp
		install_rocks
	)
	for install in "${installers[@]}"
	do
		pushd source > /dev/null
			${install}
		popd > /dev/null
	done
}

#-------------------------------------------------------

function install_system_reqs {
	reqroot
	set -e
	apt update
	apt -y upgrade
	apt -y install $debs
	set +e
}

force_download=0

function download_deps {
	if [ -d $depsdir ]; then
		echo -n "Warning: ${depsdir} already exists. "
		if [ ${force_download} -eq 0 ]; then
			echo "Download skipped."
			return
		fi
		echo ""
	fi
	mkdir -p $depsdir
	set -e
	pushd $depsdir > /dev/null
		mkdir -p source
		pushd source > /dev/null
			download_all
		popd
	popd
	set +e
}

function install_deps {
	if [ ! -d ${boost_output_dir} ]; then
		download_deps
	fi
	pushd $depsdir > /dev/null
		install_all
	popd
}

#-------------------------------------------------------

function install_main {
	cmd=$1
	shift
	if [ "_$cmd" == "_system_reqs" ]; then
		install_system_reqs $@
		exitok
	elif [ "_$cmd" == "_deps" ]; then
		set_depsdir
		install_deps $@
		exitok
	fi
}

depsdir=""
boost_output_dir=""

function download {
	cmd=$1
	shift
	set_depsdir
	if [ "_$cmd" == "_deps" ]; then
		force_download=1
		download_deps $@
		exitok
	fi
}

function make_build_dir {
	set_depsdir
	echo "building using ${jobs} jobs"
	echo "dependencies dir: ${depsdir}"
	if [ ! -d ${boost_output_dir} ]; then
		install_deps
	fi
	echo "dependencies OK at: ${depsdir}"
	sep=";"
	if [[ "$OSTYPE" == "darwin"* ]]; then
		sep=":"
	fi
	set -e
	mkdir -p _build
	pushd _build > /dev/null
		BOOST_ROOT="${depsdir}/boost" cmake .. \
		-DCMAKE_BUILD_TYPE=RelWithDebInfo \
		-DCMAKE_PREFIX_PATH="${depsdir}/facebook${sep}${depsdir}/google${sep}${depsdir}/mongodb${sep}${depsdir}/zeromq" \
		\
		-GNinja
		ninja publish
	popd
	set +e
	echo "Sources are ready in directory _build"
	echo "Compile:"
	echo "  cd _build"
	echo "  ninja -j${jobs}"
	echo "Run:"
	echo "  export LD_LIBRARY_PATH=${depsdir}/boost/lib:${depsdir}/facebook/lib:${depsdir}/google/lib:${depsdir}/mongodb/lib:${depsdir}/zeromq/lib"
	exitok
}

#-------------------------------------------------------

prog=$0
jobs=8
warn_env=0
debs="git gcc g++ cmake curl libssl-dev ninja-build pkg-config libpython-dev"

cmd=$1
shift

if [ "_$cmd" == "_install" ]; then
	install_main $@
elif [ "_$cmd" == "_download" ]; then
	download $@
elif [ "_$cmd" == "_" ]; then
	make_build_dir $@
fi

#error flow
help
exit 1


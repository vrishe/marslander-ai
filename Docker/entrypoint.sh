#!/bin/bash

branch=$1
config=$2
workdir=$3
shift 3

echo -n "Checking for updates... "
git fetch --prune --no-recurse-submodules; do_clone=$?
if (( $do_clone )); then

  echo ""

  git_url= < REDACTED >
  git clone --recurse-submodules $git_url .

else
  echo "OK"
fi

bin_path="build/$config/trainer"
commit_id=`git rev-parse $branch`
outdated=$([[ $commit_id != `git rev-parse origin/$branch` ]] && echo 'outdated')
if [[ ! -f "$bin_path" ]] || \
   [[ ! -z $outdated ]]; then

  git checkout $branch
  (( ! $do_clone )) && \
    git reset --hard origin/$branch && git pull --ff --recurse-submodules

  if [[ ! -d lib ]] || \
     ( [[ ! -z $outdated ]] && \
     (( `git diff-tree -v --name-only -r ${commit_id}..HEAD \
        | grep -c '3rd-party'` )) ); then
    chmod 544 ./setup.sh && ./setup.sh
  fi

  make clean check trainer BUILD_CONFIG=$config
fi

bin_path=$(realpath "$bin_path")
cd "$workdir"; "$bin_path"


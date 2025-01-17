#!/usr/bin/env bash
#
# Description : Build Jekyll pages with Docker and without Ruby or Gem
# How to :
# run ./build.sh in current directory
# required : Docker
# source : https://github.com/envygeeks/jekyll-docker/blob/master/README.md
#

_JEKYLL_VERSION="${JEKYLL_VERSION:-3.8}"

docker run --rm \
  -it \
  --ipc=host \
  --net=host \
  --volume="${PWD}:/srv/jekyll:Z" \
  --volume="${PWD}/vendor:/usr/local/bundle:Z" \
  jekyll/jekyll:${_JEKYLL_VERSION} \
  jekyll build

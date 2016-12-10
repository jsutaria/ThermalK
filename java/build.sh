#!/bin/sh
cd $TRAVIS_BUILD_DIR/java
sbt ++$TRAVIS_SCALA_VERSION package
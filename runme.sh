#!/bin/bash

make;

testResourcesBaseDir=./tests

baseTestDir=/tmp/vurado
testDir=$baseTestDir/brownianbot/

function setup() {
    mkdir -p $testDir;
    cp -r $testResourcesBaseDir $testDir
}

function teardown() {
    rm -r $baseTestDir;
}

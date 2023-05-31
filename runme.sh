#!/bin/bash

make;


testResourcesBaseDir=./tests

baseTestDir=/tmp/vurado
testDir=$baseTestDir/brownianbot/tests

output=./result.txt
printf "" > $output;

serverLog=$testDir/logs/server.log

serverPid="";


function printTitle() {
    printf "\n===== %s =====\n" "$1" >> $output
}

function setup() {
    rm -r $baseTestDir;

    mkdir -p $testDir/logs
    mkdir -p $testDir;
    cp -r $testResourcesBaseDir $baseTestDir/brownianbot;

    ./out/brownianbotd $testDir/configs/config > $serverLog &

    serverPid=$(pgrep brownianbotd);
}

function teardown() {
    kill -KILL "$serverPid";
}

function test100Clients() {
    printTitle "100 Clients - Try $1";

    pids=()

    for i in {0..99}; do
      ./out/brownianbot $testDir/configs/config 100 > /dev/null < $testDir/input/thousand-numbers.txt &
      printf ""
      pids[$i]=$!
    done;

    for i in {0..99}; do
      wait "${pids[$i]}";
    done;

    # shellcheck disable=SC2129
    echo "0" | ./out/brownianbot $testDir/configs/config 100 | grep "\[Request handling\]" >> $output
}

function main() {
    setup;

    printTitle "Description";

    {
        printf "Server log %s\n" "$serverLog";
        printf "Active connections history: 'cat /tmp/vurado/brownianbot/tests/logs/server.log | grep \"active_connections_count\"'\n"
        printf "Server counter history: 'cat /tmp/vurado/brownianbot/tests/logs/server.log | grep \"Updated server counter\"'\n"
        printf "Connections history (fd and heap): 'cat /tmp/vurado/brownianbot/tests/logs/server.log | grep \"Accepted\"'\n"
    } >> $output

    test100Clients 1;
    test100Clients 2;
    test100Clients 3;

    printTitle "Resources (first connection and last connection)";
    {
        grep "Accepted" < "$serverLog" | head -1;
        grep "Accepted" < "$serverLog"  | tail -1;
    } >> $output;

    teardown;
}

main;
#!/bin/bash

make;


testResourcesBaseDir=./tests

baseTestDir=/tmp/vurado
testDir=$baseTestDir/brownianbot/tests

output=./result.txt
printf "" > $output;


function printTitle() {
    printf "\n===== %s =====\n" "$1" >> $output
}

function setup() {
    rm -r $baseTestDir;

    mkdir -p $testDir/logs
    mkdir -p $testDir;
    cp -r $testResourcesBaseDir $baseTestDir/brownianbot;
}

function runClients() {
    count=$1
    delayMs=$2
    log=$3

    pids=()

    for i in $(seq 1 "$count"); do
        ./out/brownianbot $testDir/configs/config "$delayMs" > "$log" < $testDir/input/thousand-numbers.txt &
        printf ""
        pids[$i]=$!
    done;

    for i in $(seq 1 "$count"); do
        wait "${pids[$i]}";
    done;
}

function test100Clients() {
    printTitle "100 Clients - Try $1";

    runClients 100 100 /dev/null

    # shellcheck disable=SC2129
    echo "0" | ./out/brownianbot $testDir/configs/config 100 | grep "\[Request handling\]" >> $output
}

function testEfficiencyByDelayAndClientCount() {
    clientCount=$1
    delayMs=$2
    efficiencyServerLog=$3
    efficiencyClientLog=$4

    printTitle "Efficiency - Delay $delayMs ms - Client Count $clientCount";
    printf "Server log %s\n" "$efficiencyServerLog" >> $output;
    printf "Clients Merged log %s\n" "$efficiencyClientLog" >> $output;

    ./out/brownianbotd $testDir/configs/config > "$efficiencyServerLog" &
    serverPid=$(pgrep brownianbotd);

    runClients "$clientCount" "$delayMs" "$efficiencyClientLog";

    kill -KILL "$serverPid" >> /dev/null;

    firstRequestTimestamp=$(grep "Received" < "$efficiencyServerLog" | head -1 | awk '{print $2}');
    firstRequestTimestamp=${firstRequestTimestamp:1:-1}

    lastRequestTimestamp=$(grep "Received" < "$efficiencyServerLog" | tail -1 | awk '{print $2}');
    lastRequestTimestamp=${lastRequestTimestamp:1:-1}

    serverTime=$(((lastRequestTimestamp - firstRequestTimestamp) * 1000));

    maxDelayNs=$(grep "Summary delay ns" < "$efficiencyClientLog" | awk 'BEGIN{m=0} {if ($6>0+m) m=$6} END{print m}');
    # shellcheck disable=SC2004
    maxDelayMs=$((maxDelayNs / 1000000));

    maxClientTime=0;
    for i in $(seq 4 $((clientCount + 3))); do
        firstRequestTimestamp=$(grep "\[Client $i\]" < "$efficiencyServerLog" | grep "Received" | head -1 | awk '{print $2}');
        firstRequestTimestamp=${firstRequestTimestamp:1:-1}

        lastRequestTimestamp=$(grep "\[Client $i\]" < "$efficiencyServerLog" | grep "Received" | tail -1 | awk '{print $2}');
        lastRequestTimestamp=${lastRequestTimestamp:1:-1}

        clientTime=$((lastRequestTimestamp - firstRequestTimestamp));

        if [[ "$clientTime" -gt "maxClientTime" ]]; then
            maxClientTime=$clientCount;
        fi;
    done;

    {
        printf "Server Time (ms): %s\n" "$serverTime";
        printf "Max Client Time (ms): %s\n" "$maxClientTime";
        printf "Max Client Delay (ms): %s\n" "$maxDelayMs";
        printf "Efficiency: %s\n" $((serverTime - maxDelayMs));
    } >> $output;

}

function main() {
    setup;

    serverLog100Clients=$testDir/logs/server.log

    printTitle "100 Clients - Description";

    {
        printf "Server log %s\n" "$serverLog100Clients";
        printf "Active connections history: 'cat /tmp/vurado/brownianbot/tests/logs/server.log | grep \"active_connections_count\"'\n"
        printf "Server counter history: 'cat /tmp/vurado/brownianbot/tests/logs/server.log | grep \"Updated server counter\"'\n"
        printf "Connections history (fd and heap): 'cat /tmp/vurado/brownianbot/tests/logs/server.log | grep \"Accepted\"'\n"
    } >> $output

    ./out/brownianbotd $testDir/configs/config > $serverLog100Clients &
    serverPid=$(pgrep brownianbotd);

    test100Clients 1;
    test100Clients 2;
    test100Clients 3;

    kill -KILL "$serverPid" > /dev/null;

    printTitle "100 Clients - Resources (first connection and last connection)";
    {
        grep "Accepted" < "$serverLog100Clients" | head -1;
        grep "Accepted" < "$serverLog100Clients"  | tail -1;
    } >> $output;

    for c in 50 100; do
        for d in 0 200 400 600 800 1000; do
            printf "\n count %s, delay: %s ms \n" $c $d;

            printf "" > $testDir/logs/server_count"$c"_delay"$d".log;
            printf "" > $testDir/logs/client_count"$c"_delay"$d".log;

            testEfficiencyByDelayAndClientCount $c $d $testDir/logs/server_count"$c"_delay"$d".log $testDir/logs/client_count"$c"_delay"$d".log;
        done;
    done;
}

main;
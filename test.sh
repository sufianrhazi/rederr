#!/usr/bin/env bash

function sigint() {
    echo "Received SIGINT" >>/dev/stderr
}
function sigquit() {
    echo "Received SIGQUIT" >>/dev/stderr
}
function sigtstp() {
    echo "Received SIGTSTP" >>/dev/stderr
}
trap sigint SIGINT
trap sigquit SIGQUIT

echo "This line is normal"
echo "This line is also normal"
echo "but this is an error!" >>/dev/stderr
echo "This line is still normal"
echo "but this one is also an error!" >>/dev/stderr
echo "This line is very much normal"
echo "Pausing for 10 seconds..." >>/dev/stderr
sleep 10
echo "Last error" >>/dev/stderr
echo "Last line"

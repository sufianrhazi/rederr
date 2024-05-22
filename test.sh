#!/usr/bin/env bash

echo "This line is normal"
echo "This line is also normal"
echo "but this is an error!" >>/dev/stderr
echo "This line is still normal"
echo "but this one is also an error!" >>/dev/stderr
echo "This line is very much normal"

#!/bin/bash

rm -rf youku
cd src
go build -o ../youku
cd ../
unity-scope-tool youku.ini

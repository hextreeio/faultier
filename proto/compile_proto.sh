#!/bin/bash

nanopb_generator faultier.proto
protoc --python_out=. faultier.proto
protoc --python_out=. faultier.proto

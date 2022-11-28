#! /bin/bash

docker run -d --name bin/router -v "$(pwd):/opt" baseline /opt/router
#!/bin/bash

eval ${*:2} &> plop 
touch $1

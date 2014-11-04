#!/bin/bash

echo $$ >&2 && bash -c "python -c \"$1\""

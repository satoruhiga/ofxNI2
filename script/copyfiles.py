#!/usr/bin/env python

import sys
import os
import shutil

if not sys.platform in ['darwin']:
	sys.exit(-1)

if len(sys.argv) != 2:
	print '$ copylibs.py [PATH_TO_YOUR_OF_PROJECT]'
	sys.exit(1)

target_path = os.path.abspath(sys.argv[1])
os.chdir(os.path.dirname(sys.argv[0]))

if not os.path.exists(target_path):
	print 'err: project dir not found'
	sys.exit(-1)

lib_base_path = '../libs/OpenNI2/lib/osx'
lib_target_path = os.path.join(target_path, 'bin/data/OpenNI2/lib')

print 'target_project =', target_path

if not os.path.exists(lib_target_path):
	shutil.copytree(lib_base_path, lib_target_path)

config_target_path = os.path.join(target_path, 'bin/data/OpenNI2/config')
print config_target_path
if not os.path.exists(config_target_path):
	shutil.copytree('../libs/OpenNI2/config', config_target_path)

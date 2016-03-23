#!/usr/bin/env python3

import argparse, sys, subprocess, os
from pathlib import Path

class bcolors:
	HEADER = '\033[95m'
	OKBLUE = '\033[94m'
	OKGREEN = '\033[92m'
	WARNING = '\033[93m'
	FAIL = '\033[91m'
	ENDC = '\033[0m'
	BOLD = '\033[1m'
	UNDERLINE = '\033[4m'

def call(cmd, timeout):
	proc = subprocess.Popen(cmd, universal_newlines=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	succ = True
	try:
		out, err = proc.communicate(timeout=timeout)
	except subprocess.TimeoutExpired:
		succ = False
		proc.kill()

	if succ:
		return (proc.returncode, out, err)
	else:
		return (None, None, None)

def getPath(pathStr, dir):
	retPath = Path(pathStr)
	if dir and not retPath.is_dir():
		print('%s is not a valid directory name!' % pathStr)
		sys.exit(-1)
	if not dir and not retPath.is_file():
		print('%s is not a valid file name!' % pathStr)
		sys.exit(-1)
	return retPath

def ptsTest(filename, tooldir, workdir, config, k, runtime, libraries, timeout):
	filePath = getPath(filename, dir=False)
	toolPath = getPath(tooldir, dir=True)
	workPath = getPath(workdir, dir=True)
	configPath = getPath(config, dir=False)
	runtimePath = getPath(runtime, dir=False)
	if timeout <= 0:
		print('Time limit can only be a positive number')
		sys.exit(-1)

	instToolPath = toolPath.joinpath('pts-inst')
	instFilePath = workPath.joinpath(filePath.stem).with_suffix('.inst.bc')
	instExecPath = workPath.joinpath(filePath.stem).with_suffix('.inst')
	checkToolPath = toolPath.joinpath('pts-verify')
	os.environ['LOG_DIR'] = str(workPath)
	logPath = workPath.joinpath('pts.log')

	instCmd = [str(instToolPath), str(filePath), '-o', str(instFilePath), '-ptr-config', str(configPath), '-no-prepass']
	subprocess.call(instCmd)

	compileCmd = ['clang', str(instFilePath), str(runtimePath), '-o', str(instExecPath)]
	if libraries is not None:
		for lib in libraries:
			compileCmd.append('-l' + lib)
	subprocess.call(compileCmd)

	runCmd = [str(instExecPath)]
	callret = call(runCmd, timeout=timeout)
	if callret is None:
		print('Program timeout')
		sys.exit(-2)

	checkCmd = [str(checkToolPath), str(filePath), str(logPath), '-k', str(k), '-ptr-config', str(configPath)]

	try:
		callret, out, err = call(checkCmd, timeout)
	except subprocess.CalledProcessError as e:
		print(e.output)
		print(bcolors.FAIL + 'Analysis crashed' + bcolors.ENDC)
		sys.exit(-3)

	if callret is None:
		print('Analysis timeout')
		sys.exit(-2)

	if not callret == 0:
		print(bcolors.FAIL + 'Test failed. Error output:' + bcolors.ENDC)
		print(err)
		sys.exit(-1)
	else:
		print(bcolors.OKGREEN + 'Test passed' + bcolors.ENDC)

if __name__ == "__main__":
	optionParser = argparse.ArgumentParser(description='Pointer analysis testing tool')
	optionParser.add_argument('filename', help='the input LLVM IR file name')
	optionParser.add_argument('runtime', help='specify where the runtime library is')
	optionParser.add_argument('-w', '--workdir', help='specify the working directory', default='./', type=str)
	optionParser.add_argument('-b', '--tooldir', help='specify the directory that contains TPA tools', default='bin/', type=str)
	optionParser.add_argument('-c', '--config', help='specify the pointer annotation config file', default='ptr.config', type=str)
	optionParser.add_argument('-k', '--context', help='specify the context limit', default=0, type=int)
	optionParser.add_argument('-t', '--timeout', help='time limit for the program (in seconds)', type = int, default = 3)
	optionParser.add_argument('-l', '--library', help='Additional libraries used during linking', action='append')
	args = optionParser.parse_args()

	ptsTest(args.filename, args.tooldir, args.workdir, args.config, args.context, args.runtime, args.library, args.timeout)
	sys.exit(0)
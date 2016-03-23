#!/usr/bin/env python3

import os, sys, argparse, subprocess
from pathlib import Path

def getPath(pathStr, dir):
	retPath = Path(pathStr)
	if dir and not retPath.is_dir():
		print('%s is not a valid directory name!' % pathStr)
		sys.exit(-1)
	if not dir and not retPath.is_file():
		print('%s is not a valid file name!' % pathStr)
		sys.exit(-1)
	return retPath

def compile(inFile, outFile, toolDir, isPrepassDisabled, includeDir, defineStrs):
	inputPath = getPath(inFile, dir=False)
	toolPath = getPath(toolDir, dir=True)
	if outFile == '':
		outPath = inputPath.with_suffix('.ll')
	else:
		outPath = Path(outFile)

	compileCmd = ['clang', str(inputPath), '-Wno-everything', '-emit-llvm', '-S', '-o', str(outPath)]
	if includeDir != '':
		compileCmd += ['-I', includeDir]
	if defineStrs is not None:
		for dstr in defineStrs:
			compileCmd.append('-D' + dstr)
	subprocess.call(compileCmd)

	optCmd = ['opt', str(outPath), '-mem2reg', '-instnamer', '-S', '-o', str(outPath)]
	subprocess.call(optCmd)

	if isPrepassDisabled:
		return

	preToolPath = toolPath.joinpath('global-pts')
	prepassCmd = [str(preToolPath), str(outPath), '-o', str(outPath)]
	subprocess.call(prepassCmd, stdout=subprocess.DEVNULL)

if __name__ == "__main__":
	optionParser = argparse.ArgumentParser(description='Script that compile a C file using clang and then canonicalize the result IR for TPA')
	optionParser.add_argument('filename', help='specify the LLVM IR input file name')
	optionParser.add_argument('-b', '--tooldir', help='specify the directory containing all TPA tools', default='bin', type=str)
	optionParser.add_argument('-o', '--output', help='specify output file name', default='', type=str)
	optionParser.add_argument('-n', '--no-prepass', help='Do not run canonicalization prepass. Just mem2reg and instnamer', action='store_true')
	optionParser.add_argument('-I', '--include', help='Additional include dir', default='', type=str)
	optionParser.add_argument('-D', '--define', help='Additional #define for compilation', action='append')
	arg = optionParser.parse_args()

	compile(arg.filename, arg.output, arg.tooldir, arg.no_prepass, arg.include, arg.define)
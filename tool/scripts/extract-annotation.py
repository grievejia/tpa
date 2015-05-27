#!/usr/bin/env python3

import argparse, sys

def getFunName(line):
	words = line.split()
	assert len(words) > 1

	if words[0] in ['IGNORE', 'SOURCE', 'PIPE', 'SINK']:
		return words[1]
	else:
		return words[0]

if __name__ == "__main__":
	optionParser = argparse.ArgumentParser(description='Extract external annotations for specified functions from a given file')
	optionParser.add_argument('annot_file', help='specify the name of the annotation file')
	optionParser.add_argument('-f', '--func_file', help='specify the name of the file containing all function names (default: stdin)', default='-')
	args = optionParser.parse_args()

	funDict = dict()
	with open(args.annot_file, 'r') as annotFile:
		for line in annotFile:
			line = line.strip()
			if len(line) == 0 or line.startswith('#'):
				continue

			funName = getFunName(line)
			if funName in funDict:
				funDict[funName].append(line)
			else:
				funDict[funName] = [line]

	funFile = sys.stdin if args.func_file == '-' else open(args.func_file)
	missingFuncs = []
	for line in funFile:
		line = line.strip()
		if line in funDict:
			for entry in funDict[line]:
				print(entry)
		else:
			missingFuncs.append(line)
	if funFile is not sys.stdin:
		funFile.close()

	if len(missingFuncs) > 0:
		print('\nMissing functions:', file=sys.stderr)
		for entry in missingFuncs:
			print(entry, file=sys.stderr)
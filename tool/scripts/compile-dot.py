#!/usr/bin/env python3

import os, sys, argparse
from pathlib import Path

if __name__ == "__main__":
	optionParser = argparse.ArgumentParser(description='Compile all .dot files under a given directory to PNG files')
	optionParser.add_argument('dir', help='specify the directory containing .dot files')
	arg = optionParser.parse_args()

	filePath = Path(arg.dir)
	if not filePath.is_dir():
		print('%s is not a valid path name!' % arg.dir)
		sys.exit(-1)

	for dotFile in filePath.glob('*.dot'):
		print('Compiling %s' % dotFile.name)

		cmd = 'dot -Tpng ' + str(dotFile) + ' -o ' + str(dotFile.with_suffix('.png'))
		os.system(cmd)

	sys.exit(0)
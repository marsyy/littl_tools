#!/usr/bin/python
import os
import struct
import lz4.frame
from optparse import OptionParser



def get_lz4_file(Image_fit):
	with open(Image_fit, "r") as fit_fp:
		fit_fp.read(0xB0)
		kernel_lenth = struct.unpack(">I", fit_fp.read(4))[0]
		fit_fp.read(4)
		lz4_file = fit_fp.read(kernel_lenth)
		return lz4_file
		

def get_image(Image_fit, Image):
	lz4_file = get_lz4_file(Image_fit)
	Image_file = lz4.frame.decompress(lz4_file)

	with open(Image, "w") as Image_fp:
		Image_fp.write(Image_file)


def main():
	parser = OptionParser(usage='usage:get_image <imagefit_file> <image_file>')
	(options,args) = parser.parse_args()
	if len(args) < 2:
		parser.error("incorrect number of arguments")
	get_image(args[0],args[1])

if __name__ == "__main__":
	main()

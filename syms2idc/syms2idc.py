#!/usr/bin/python
import os
from optparse import OptionParser



def syms2idc(in_file,out_file):
	idc_header = "#include <idc.idc>\n"
	idc_header+="static main()\n"
	idc_header+="{\n"
	idc_body = ""
	idc_foot = "\n}"
	tmp = ""
	try:
		f_in = open(in_file,"r")
		f_out = open(out_file,"w")
	except IOError,e:
		print e
	try:
		for line in f_in:
			data_list = line.split()
			tmp = "\tMakeNameEx(0x%s,\"%s\",0);\n"%(data_list[0],data_list[2])
			idc_body+=tmp
		f_out.write(idc_header+idc_body+idc_foot)
	except IOError,e:
		print e
	finally:
		f_in.close()
		f_out.close()



def main():
	parser = OptionParser(usage='usage:syms2idc <syms_file> <idc_file>')
	(options,args) = parser.parse_args()
	if len(args) < 2:
		parser.error("incorrect number of arguments")
	syms2idc(args[0],args[1])

if __name__ == "__main__":
	main()

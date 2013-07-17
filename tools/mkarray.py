#!/usr/bin/env python
#
# Copyright (c) 2011 Martin Decky
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# - Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# - The name of the author may not be used to endorse or promote products
#   derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
"""
C structure creator
"""

import sys
import os
import struct

def usage(prname):
	"Print usage syntax"
	print("%s <DESTINATION> <LABEL> [SOURCE ...]" % prname)

def main():
	if (len(sys.argv) < 3):
		usage(sys.argv[0])
		return
	
	dest = sys.argv[1]
	label = sys.argv[2]
	
	header_ctx = []
	data_ctx = []
	for src in sys.argv[3:]:
		basename = os.path.basename(src)
		symbol = basename.replace(".", "_")
		
		print("%s -> %s" % (src, symbol))
		
		src_in = open(src, "rb")
		src_data = src_in.read()
		src_in.close()
		
		header_rec = "extern uint8_t %s[];" % symbol
		header_ctx.append(header_rec)
		
		data_rec = "uint8_t %s[] = {\n\t" % symbol
		
		fmt = 'B'
		item_size = struct.calcsize(fmt)
		offset = 0
		cnt = 0
		
		while (len(src_data[offset:]) >= item_size):
			byte = struct.unpack_from(fmt, src_data, offset)
			
			if (offset > 0):
				if ((cnt % 15) == 0):
					data_rec += ",\n\t"
				else:
					data_rec += ", "
			
			data_rec += "0x%x" % byte
			offset += item_size
			cnt += 1
		
		data_rec += "\n};\n"
		data_ctx.append(data_rec)
		
		header_rec = "extern size_t %s_size;" % symbol
		header_ctx.append(header_rec)
		
		data_rec = "size_t %s_size = %u;\n" % (symbol, offset)
		data_ctx.append(data_rec)
	
	header = open("%s.h" % dest, "w")
	
	header.write('/***************************************\n')
	header.write(' * AUTO-GENERATED FILE, DO NOT EDIT!!! *\n')
	header.write(' ***************************************/\n\n')
	header.write("#ifndef %s_H_\n" % label)
	header.write("#define %s_H_\n\n" % label)
	header.write("#include <sys/types.h>\n\n")
	header.write("\n".join(header_ctx))
	header.write("\n\n")
	header.write("#endif\n")
	
	header.close()
	
	data = open("%s.c" % dest, "w")
	
	data.write('/***************************************\n')
	data.write(' * AUTO-GENERATED FILE, DO NOT EDIT!!! *\n')
	data.write(' ***************************************/\n\n')
	data.write("#include \"%s.h\"\n\n" % dest)
	data.write("\n".join(data_ctx))
	
	data.close()

if __name__ == '__main__':
	main()

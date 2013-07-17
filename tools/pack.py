#!/usr/bin/env python
#
# Copyright (c) 2008 Martin Decky
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
Binary package creator
"""

import sys
import os
import subprocess
import zlib
import shutil

SANDBOX = 'pack'
LINK = '_link.ld'
COMPONENTS = '_components'

def usage(prname):
	"Print usage syntax"
	print("%s <OBJCOPY> <FORMAT> <ARCH> <ARCH_PATH> [COMPONENTS ...]" % prname)

def deflate(data):
	"Compress using deflate algorithm (without any headers)"
	return zlib.compress(data, 9)[2:-4]

def print_error(msg):
	"Print a bold error message"
	
	sys.stderr.write("\n")
	sys.stderr.write("######################################################################\n")
	sys.stderr.write("HelenOS build sanity check error:\n")
	sys.stderr.write("\n")
	sys.stderr.write("%s\n" % "\n".join(msg))
	sys.stderr.write("######################################################################\n")
	sys.stderr.write("\n")
	
	sys.exit(1)

def sandbox_enter():
	"Create a temporal sandbox directory for packing"
	
	if (os.path.exists(SANDBOX)):
		if (os.path.isdir(SANDBOX)):
			try:
				shutil.rmtree(SANDBOX)
			except:
				print_error(["Unable to cleanup the directory \"%s\"." % SANDBOX])
		else:
			print_error(["Please inspect and remove unexpected directory,",
			             "entry \"%s\"." % SANDBOX])
	
	try:
		os.mkdir(SANDBOX)
	except:
		print_error(["Unable to create sandbox directory \"%s\"." % SANDBOX])
	
	owd = os.getcwd()
	os.chdir(SANDBOX)
	
	return owd

def sandbox_leave(owd):
	"Leave the temporal sandbox directory"
	
	os.chdir(owd)

def main():
	if (len(sys.argv) < 5):
		usage(sys.argv[0])
		return
	
	objcopy = sys.argv[1]
	format = sys.argv[2]
	arch = sys.argv[3]
	arch_path = sys.argv[4]
	
	header_ctx = []
	data_ctx = []
	link_ctx = []
	cnt = 0
	for component in sys.argv[5:]:
		basename = os.path.basename(component)
		plainname = os.path.splitext(basename)[0]
		obj = "%s.co" % plainname
		symbol = "_binary_%s" % basename.replace(".", "_")
		
		print("%s -> %s" % (component, obj))
		
		comp_in = open(component, "rb")
		comp_data = comp_in.read()
		comp_in.close()
		
		comp_deflate = deflate(comp_data)
		
		owd = sandbox_enter()
		
		try:
			comp_out = open(basename, "wb")
			comp_out.write(comp_deflate)
			comp_out.close()
			
			subprocess.call([objcopy,
				"-I", "binary",
				"-O", format,
				"-B", arch,
				"--rename-section", ".data=.%s_image" % plainname,
				basename, os.path.join(owd, obj)])
			
		finally:
			sandbox_leave(owd)
		
		link_ctx.append("\t\t*(.%s_image);" % plainname)
		
		header_rec = "extern int %s_start;\n" % symbol
		header_rec += "extern int %s_size;\n" % symbol
		header_ctx.append(header_rec)
		
		data_rec = "\t{\n"
		data_rec += "\t\t.name = \"%s\",\n" % plainname
		data_rec += "\t\t.start = (void *) &%s_start,\n" % symbol
		data_rec += "\t\t.size = (size_t) &%s_size,\n" % symbol
		data_rec += "\t\t.inflated = %d\n" % len(comp_data)
		data_rec += "\t}"
		data_ctx.append(data_rec)
		
		cnt += 1
	
	header = open(os.path.join(arch_path, "include", "%s.h" % COMPONENTS), "w")
	
	header.write('/***************************************\n')
	header.write(' * AUTO-GENERATED FILE, DO NOT EDIT!!! *\n')
	header.write(' ***************************************/\n\n')
	header.write("#ifndef BOOT_COMPONENTS_H_\n")
	header.write("#define BOOT_COMPONENTS_H_\n\n")
	header.write("#include <typedefs.h>\n\n")
	header.write("#define COMPONENTS  %d\n\n" % cnt)
	header.write("typedef struct {\n")
	header.write("\tconst char *name;\n")
	header.write("\tvoid *start;\n")
	header.write("\tsize_t size;\n")
	header.write("\tsize_t inflated;\n")
	header.write("} component_t;\n\n")
	header.write("extern component_t components[];\n\n")
	header.write("\n".join(header_ctx))
	header.write("\n")
	header.write("#endif\n")
	
	header.close()
	
	data = open(os.path.join(arch_path, "src", "%s.c" % COMPONENTS), "w")
	
	data.write('/***************************************\n')
	data.write(' * AUTO-GENERATED FILE, DO NOT EDIT!!! *\n')
	data.write(' ***************************************/\n\n')
	data.write("#include <typedefs.h>\n")
	data.write("#include <arch/%s.h>\n\n" % COMPONENTS)
	data.write("component_t components[] = {\n")
	data.write(",\n".join(data_ctx))
	data.write("\n")
	data.write("};\n")
	
	data.close()
	
	link_in = open(os.path.join(arch_path, "%s.in" % LINK), "r")
	template = link_in.read()
	link_in.close()
	
	link_out = open(os.path.join(arch_path, "%s.comp" % LINK), "w")
	link_out.write(template.replace("[[COMPONENTS]]", "\n".join(link_ctx)))
	link_out.close()

if __name__ == '__main__':
	main()

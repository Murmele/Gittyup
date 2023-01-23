import string
import sys
import os
import iface

ptype_lookup = {
	"bool": "bool",
	"cells": "const QString &",
	"colour": "QColor",
	"findtext": "Sci_TextToFind *",
	"formatrange": "Sci_RangeToFormat *",
	"int": "int",
	"keymod": "int",
	"position": "int",
	"string": "const QString &",
	"stringresult": "char *",
	"textrange": "Sci_TextRange *",
}

def Contains(s, sub):
	return sub in s

def pType(s):
	return ptype_lookup.get(s, "")

def printDefines(f, out):
	for name in f.order:
		v = f.features[name]
		if v["Category"] != "Deprecated":
			if v["FeatureType"] in ["fun", "get", "set"]:
				featureDefineName = "SCI_" + string.upper(name)
				out.write("#define " + featureDefineName + " " + v["Value"] + ";\n")
			elif v["FeatureType"] in ["evt"]:
				featureDefineName = "SCN_" + string.upper(name)
				out.write("#define " + featureDefineName + " " + v["Value"] + ";\n")
			elif v["FeatureType"] in ["val"]:
				featureDefineName = string.upper(name)
				out.write("#define " + featureDefineName + " " + v["Value"] + ";\n")

def genfuncPrototype(name, fun):
	retval = name.replace("Get", "")
	retval = retval[0].lower() + retval[1:]
	retval = retval + "("
	bp1 = 0
	if fun["Param1Type"] != "" and fun["Param1Type"] != " ":
		# Disregard string length information.
		temp1 = fun["Param1Type"] != "int" or fun["Param1Name"] != "length"
		temp2 = fun["Param2Type"] != "string" or fun["Param2Name"] != "text"
		if temp1 or temp2:
			name = fun["Param1Name"]
			type = pType(fun["Param1Type"])
			if name == "utf8":
				type = "const char *"
			retval = retval + type + " " + name
			bp1 = 1
	if fun["Param2Type"] != "" and fun["Param2Type"] != " ":
		if bp1 == 1:
			retval = retval + ", "
		name = fun["Param2Name"]
		type = pType(fun["Param2Type"])
		if name == "pointer":
			type = "sptr_t"
		elif name == "xpmData" or name == "pixmap":
			type = "const char *"
		retval = retval + type + " " + name
	retval = retval + ")"
	return retval

def getfuncHeader(name, fun, extra):
	retval = genfuncPrototype(name, fun)
	line = ""
	if pType(fun["ReturnType"]) == "":
		line = line + "void " + extra + retval
	else:
		rettype = pType(fun["ReturnType"])
		if name == "GetDocPointer" or name == "GetCharacterPointer":
			rettype = "sptr_t "
		line = line + rettype + " " + extra + retval
		if fun["FeatureType"] == "get":
			line = line + " const"
	return line

def printFunctionDefs(f, out):
	for name in f.order:
		v = f.features[name]
		if v["Category"] != "Deprecated":
			if v["FeatureType"] in ["fun", "set", "get"]:
				line = ""
				if not v["Comment"] in ["", " "]:
					line = line + "		/**\n"
				
					for cline in v["Comment"]:
						line = line + "		 * " + cline + "\n";
					line = line + "		 */\n		"
				else:
					line = "		"
				line = line + getfuncHeader(name, v, "") + ";\n"
				out.write(line)

def isFunction(v):
	return pType(v["ReturnType"]) != ""

def printFunctionImpl(f, out):
	for name in f.order:
		v = f.features[name]
		if v["Category"] != "Deprecated":
			if v["FeatureType"] in ["fun", "set", "get"]:
				header = getfuncHeader(name, v, "ScintillaIFace::") + "\n"
				out.write(header)
				out.write("{\n")
				line = "	"
				endline = ""
				
				# Convert just once in case both length and data are needed.
				utf8 = v["Param1Name"] == "utf8"
				ptr = v["Param2Name"] == "pointer"
				xpm = v["Param2Name"] == "xpmData" or v["Param2Name"] == "pixmap"
				if pType(v["Param1Type"]) == "const QString &" and not utf8:
					out.write("\tQByteArray " + v["Param1Name"] + "Utf8 = " +
					          v["Param1Name"] + ".toUtf8();\n")
				if pType(v["Param2Type"]) == "const QString &" and not xpm:
					out.write("\tQByteArray " + v["Param2Name"] + "Utf8 = " +
					          v["Param2Name"] + ".toUtf8();\n")
				
				if not isFunction(v):
					line = line + "send("
				else:
					line = line + "return "
					if pType(v["ReturnType"]) != "long":
						if pType(v["ReturnType"]) != "bool":
							if pType(v["ReturnType"]) == "QColor":
								line = line + "decodeColor("
							elif name != "GetDocPointer" and name != "GetCharacterPointer":
								line = line + "(" + pType(v["ReturnType"]) + ")"
						else:
							endline = " != 0"
					line = line + "send("
				line = line + "SCI_" + string.upper(name) + ", "
				
				if not (v["Param1Type"] in ["", " "]):
					# Disregard string length information.
					temp1 = v["Param1Type"] != "int" or v["Param1Name"] != "length"
					temp2 = v["Param2Type"] != "string" or v["Param2Name"] != "text"
					if temp1 or temp2:
						if pType(v["Param1Type"]) == "const QString &" and not utf8:
							line = line + "(uptr_t)"
							line = line + v["Param1Name"] + "Utf8.constData(), "
						elif pType(v["Param1Type"]) == "QColor":
							line = line + "encodeColor("
							line = line + v["Param1Name"] + "), "
						else:
							if pType(v["Param1Type"]) != "long":
								line = line + "(uptr_t)"
							line = line + v["Param1Name"] + ", "
					else:
						line = line + "(uptr_t)"
						line = line + v["Param2Name"] + "Utf8.length(), "
				else:
					line = line + "0, "
					
				if not (v["Param2Type"] in ["", " "]):
					if pType(v["Param2Type"]) == "const QString &" and not xpm:
						line = line + "(sptr_t)"
						line = line + v["Param2Name"] + "Utf8.constData())"
					elif pType(v["Param2Type"]) == "QColor":
						line = line + "encodeColor("
						line = line + v["Param2Name"] + "))"
					else:
						if pType(v["Param2Type"]) != "long" and not ptr:
							line = line + "(sptr_t)"
						line = line + v["Param2Name"] + ")"
				else:
					line = line + "0)"
				if pType(v["ReturnType"]) == "QColor":
					line = line + ")"
				line = line + endline + ";"
				out.write(line + "\n}\n\n")

def printInlineFunctionImpl(f, out):
	for name in f.order:
		v = f.features[name]
		if v["Category"] != "Deprecated":
			if v["FeatureType"] in ["fun", "set", "get"]:
			
				line = ""
				indent = "\t\t"
				indent2 = "\t\t\t"
			
				if not v["Comment"] in ["", " "]:
					line = indent + "/**\n";
					
					for cline in v["Comment"]:
						line = line + indent + " * " + cline + "\n";
					line = line + indent + " */\n";
				
				out.write(line)
			
				header = getfuncHeader(name, v, "") + "\n"
				out.write(indent + header)
				out.write(indent + "{\n")
				
				line = ""
				endline = ""
				if not isFunction(v):
					line = line + "send("
				else:
					line = line + "return "
					if pType(v["ReturnType"]) != "long":
						if pType(v["ReturnType"]) != "bool":
							line = line + "(" + pType(v["ReturnType"]) + ")"
						else:
							endline = " != 0"
					line = line + "send("
				line = line + "SCI_" + string.upper(name) + ", "
				if not (v["Param1Type"] in ["", " "]):
					if pType(v["Param1Type"]) != "long":
						line = line + "(uptr_t)"
					line= line + v["Param1Name"] + ", "
				else:
					line = line + "0, "
				if not (v["Param2Type"] in ["", " "]):
					if pType(v["Param2Type"]) != "long":
						line = line + "(sptr_t)"
					line = line + v["Param2Name"] + ")"
				else:
					line = line + "0)"
				line = line + endline + ";"
				out.write(indent2 + line + "\n" + indent + "}\n\n")

def genMainControl(input, output, definition):
	copying = 1
	for line in input.readlines():
		if copying:
			output.write(line)
		if Contains(line, "//++EventTypes"):
			copying = 0
			printEventDefs(definition, output)
		if Contains(line,"//++EventPrivates"):
			copying = 0
			printEventPrivates(definition, output)
		if Contains(line, "//++EventProperties"):
			copying = 0
			printEventProperties(definition, output)
		if Contains(line,"//++FuncDef"):
			copying = 0
			printFunctionDefs(definition, output)
		if Contains(line,"//++Const"):
			copying = 0
			printDefines(definition, output)
		if Contains(line,"//++FuncImp"):
			copying = 0
			printFunctionImpl(definition, output)
		if Contains(line,"//++InlineFuncImp"):
			copying = 0
			printInlineFunctionImpl(definition, output)
		if Contains(line,"//++EventImpl"):
			copying = 0
			printEventImpl(definition, output)
		if Contains(line,"//--"):
			copying = 1
			output.write(line)

def genConsts(input, output, definition):
	copying = 1
	for line in input.readlines():
		if copying:
			output.write(line)
		if Contains(line,"//++Const"):
			copying = 0
			printDefines(definition, output)
		if Contains(line,"//--") and copying == 0:
			copying = 1
			output.write(line)

def regenerate(filename, outputfilename, definition, fn):
	tempname = outputfilename + ".tmp"
	out = open(tempname,"w")
	hfile = open(filename)
	fn(hfile, out, definition)
	out.close()
	hfile.close()
	if(os.access(outputfilename, os.F_OK)):
		os.unlink(outputfilename)
	os.rename(tempname, outputfilename)

# Program Start
f = iface.Face()
f.ReadFromFile(sys.argv[1])
regenerate("ScintillaIFace.cpp.in", "ScintillaIFace.cpp", f, genMainControl)
regenerate("ScintillaIFace.h.in", "ScintillaIFace.h", f, genMainControl)

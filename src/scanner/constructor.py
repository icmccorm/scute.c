import sys, getopt, pprint
args = sys.argv

helpString = "constructor.py -c <constantlist> -k <keywordlist> --pc <pre-cfile> --ph <pre-headerfile> --pre-enum-key --pre-enum-const="

def main(argv):
	inputKeywords = ''
	inputConstants = ''
	preHeader = ''
	preCfile = ''
	outputName = 'tokenizer'
	try:
		opts, args = getopt.getopt(argv, 'hc:k:o:')
	except getopt.GetoptError:
		print(helpString)
		sys.exit(2)
	for opt, arg in opts:
		if opt =='-h':
			print(helpString)
			sys.exit()
		elif opt == "-c":
			inputConstants = arg
		elif opt == "-k":
			inputKeywords = arg
		elif opt == "--ph":
			preHeader = arg
		elif opt == "--pc":
			preCfile = arg

	if inputKeywords == '' or inputConstants == '' or outputName == '':
		print(helpString)
		sys.exit(2)

	inputKeywords = open(inputKeywords, "r").read()
	inputConstants = open(inputConstants, "r").read()

	if preCfile != '':
		preCfile = open(preCfile, "r").read()
	if preHeader != '':
		preHeader = open(preHeader, "r").read()

	hFile = getHeaderStart(outputName) + preHeader + "\n"
	cFile = preCfile + "\n"

	sourceList = buildDict(inputKeywords, inputConstants, outputName)
	hFile += sourceList[0]
	cFile += "#include \"" + outputName + ".h\"\n" + sourceList[1]

	hFile += getHeaderClose()

	dest = open(outputName + ".h", "w")
	dest.write(hFile)

	dest = open(outputName + ".c", "w")
	dest.write(cFile)	

def buildDict(keys, constants, filename):
	hFile = ""
	cFile = ""
	keyArray = keys.split("\n")
	constArray = constants.split("\n")

	keyBundle = createBundle("TK", keyArray)
	hFile += keyBundle[0]

	constBundle = createBundle("CS", constArray)
	hFile += constBundle[0]

	keywordScanner = buildScannerFunction("TK", "Keyword", "ID", keyBundle[1])
	constantScanner = buildScannerFunction("CS", "Constant", "ERROR", constBundle[1])

	hFile += keywordScanner[0] + "\n" + constantScanner[0] + "\n"
	cFile += keywordScanner[1] + "\n" + constantScanner[1] + "\n"

	return [hFile, cFile]


def createBundle(name, list):
	enumTxt = "typedef enum {\n"
	switchMap = {}
	for item in list:
		if len(item) > 0:	
			if(item[0] == ":"):
				enumTxt += "\t" + name.upper() + "_" + item[1:].upper() + ",\n"
			else:
				enumTxt += "\t" + name.upper() + "_" + item.upper() + ",\n"
				insertItem(switchMap, item)
	pprint.pprint(switchMap)
	enumTxt += "} " + name.upper() + "Type;\n\n"

	return [enumTxt, switchMap]

def insertItem(map, chars):
	if chars[0] not in map:
		map[chars[0]] = chars[1:]
	elif not type(map[chars[0]]) == str:
		insertItem(map[chars[0]], chars[1:])
	else:
		existingString = map[chars[0]]
		endOfCurrentString = chars[1:]
		if(len(existingString) > 0 and len(endOfCurrentString) > 0):
			if(existingString[0] == endOfCurrentString[0]):
				map[chars[0]] = {}
				insertItem(map[chars[0]], endOfCurrentString)
				insertItem(map[chars[0]], existingString)
			else:
				map[chars[0]] = {existingString[0]: existingString[1:], endOfCurrentString[0]: endOfCurrentString[1:]}
		else:	
			if(len(existingString) > 0):
				map[chars[0]] = {'': None, existingString: None}
			else:
				map[chars[0]] = {'': None, endOfCurrentString: None}


def buildScannerFunction(returnType, name, defaultReturn, itemDict):
	methodTag = returnType + "Type find" + name + "(char* start, char* current)"
	functionBody = methodTag + "{\n"
	functionBody += "\tint length = current - start;\n"
	functionBody += "\tswitch(start[0]){\n"
	functionBody += buildLevel(itemDict, 1, "", "check" + name, returnType, defaultReturn)
	functionBody += "\t}\n}"
	return [methodTag + ";\n", functionBody]

def buildLevel(currLevel, numIndents, currWord, checkMethod, enumHeader, defaultEnumVal):
	levelText = ""
	tab = (numIndents + (numIndents - 1) * 2) * "\t"
	tabIn = tab + '\t'
	if type(currLevel) == dict:
		for key in currLevel:
			nextLevelIn = currLevel[key]
			if type(nextLevelIn) == str:
				levelText += tabIn + "case '" + key + "':"
				enumItem = enumHeader + "_" + (currWord + key + nextLevelIn).upper()
				if(len(nextLevelIn) != 0):
					levelText += " return " + checkMethod + "(" + str(numIndents) + ", " + str(len(nextLevelIn)) + ", \"" + nextLevelIn + "\"" + ", " + enumItem + ");"
				else:
					levelText += " return " + enumItem + ";"
			elif nextLevelIn == None:
				enumItem = enumHeader + "_" + (currWord + key).upper()
				if len(key) > 0:
					levelText += tabIn + "case '" + key[0] + "':{\n"
					if len(key) > 1:
						levelText += tabIn + "\tif(length > " + str(numIndents) + " && start[" + str(numIndents) + "] == '" + key[1] + "'){\n"
						if(len(key) > 2):
							levelText += tabIn + "\t\treturn " + checkMethod + "(" + str(numIndents) + ", " + str(len(key)) + ", \"" + key[2:] + "\"" + ", " + enumItem + ");\n"
						else:
							levelText += tabIn + "\t\treturn " + enumHeader.upper() + "_" + (currWord + key).upper() + ";\n"
						levelText += tabIn + "\t}\n"
					else:
						levelText += tabIn + "\t" + "return " + enumItem + ";\n"
					levelText += tabIn + "} break;\n"
					if numIndents == 1: levelText += tabIn + "return " + enumHeader.upper() + "_" + defaultEnumVal.upper() + "\n"
			else:
				levelText += tabIn + "case '" + key + "':"
				levelText += "{\n"
				levelText += tabIn + "\t" + "if(length > " + str(numIndents) + "){\n"
				levelText += tabIn + "\t\t" + "switch(start[" + str(numIndents) + "]){\n"
				levelText += buildLevel(nextLevelIn, numIndents + 1, currWord + key, checkMethod, enumHeader, defaultEnumVal)
				levelText += tabIn + "\t\t}\n" 
				if '' in nextLevelIn:
					levelText += tabIn + "\t}else{\n" + tabIn + '\t\treturn ' + enumHeader.upper() + "_" + (currWord + key).upper() + ";\n"
				levelText += tabIn + "\t}\n"
				if numIndents == 1:
					levelText += tabIn + "return " + enumHeader.upper() + "_" + defaultEnumVal.upper() + ";\n"
				levelText+=tabIn+"} break;\n"
			levelText += "\n"
	return levelText

def getHeaderStart(name):
	return "#ifndef scute_" + name + "_h\n#define scute_" + name + "_h\n"

def getHeaderClose():
	return "#endif"

if __name__ == "__main__":
	main(sys.argv[1:])
		
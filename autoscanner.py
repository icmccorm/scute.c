import sys, getopt, pprint, re, os
args = sys.argv

helpString = "constructor.py -c <constantlist> -k <keywordlist> -o <outputfolder>"

keywordLineMatcher = re.compile("\A(:?[A-Za-z_]*)(\[[A-Za-z_ ]*?,[A-Za-z_ ]*?,[A-Za-z_ ]*?\])?\Z")

parserOutputName = 'parsemap'
tokenizerOutputName = "tokenizer"

def main(argv):
	inputKeywords = ''
	inputConstants = ''
	outputDir = ''
	try:
		opts, args = getopt.getopt(argv, 'hc:k:d:')
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
		elif opt == "-d":
			outputDir = arg

	if inputKeywords == '' or inputConstants == '' or outputDir == '':
		print(helpString)
		sys.exit(2)

	outputDir = os.path.realpath(outputDir).strip()

	try:
		os.chdir(outputDir)
	except PermissionError:
		print("Access Denied to \"" + outputDir + "\".")
		sys.exit(3)

	try:
		inputKeywords = open(inputKeywords, "r").read()
	except IOError:
		print("File " + inputKeywords + " does not exist in specified working directory " + outputDir)
		sys.exit(4)

	try:
		inputConstants = open(inputConstants, "r").read()
	except IOError:
		print("File " + inputConstants + " does not exist in specified working directory " + outputDir)
		sys.exit(5)

	tokenizerHFile = getHeaderStart(tokenizerOutputName) + "\n"
	tokenizerCFile = ""

	parsemapHFile = getHeaderStart(parserOutputName) + "\n"
	parsemapCFile = ""

	sourceList = buildDict(inputKeywords, inputConstants, tokenizerOutputName)

	tokenizerHFile += sourceList[0]
	tokenizerCFile += "#include \"" + tokenizerOutputName + ".h\"\n" + sourceList[1]

	parsemapHFile += sourceList[2]
	parsemapCFile += "#include \"" + parserOutputName + ".h\"\n" + "#include \"compiler_defs.h\"\n" + sourceList[3]

	tokenizerHFile += getHeaderClose()
	parsemapHFile += getHeaderClose()
	
	dest = open(parserOutputName + ".c", "w").write(parsemapCFile)
	dest = open(parserOutputName + ".h", "w").write(parsemapHFile)

	dest = open(tokenizerOutputName + ".c", "w").write(tokenizerCFile)
	dest = open(tokenizerOutputName + ".h", "w").write(tokenizerHFile)

def buildDict(keys, constants, filename):
	tokenizerHFile = ""
	tokenizerCFile = "\n#include \"scanner.h\"\n"
	parsemapHFile = ""
	parsemapCFile = ""

	keyParsingArray = generateParsingArray(keys.split("\n"))
	keyArray = keyParsingArray[0]
	parsemapHFile += keyParsingArray[1]
	parsemapCFile += keyParsingArray[2]

	constArray = constants.split("\n")

	keyBundle = createBundle("TK", keyArray)
	tokenizerHFile += keyBundle[0]

	constBundle = createBundle("CS", constArray)
	tokenizerHFile += constBundle[0]

	keywordScanner = buildScannerFunction("TK", "Keyword", "ID", keyBundle[1])
	constantScanner = buildScannerFunction("CS", "Constant", "ERROR", constBundle[1])

	tokenizerHFile += keywordScanner[0] + "\n" + constantScanner[0] + "\n"
	tokenizerCFile += keywordScanner[1] + "\n" + constantScanner[1] + "\n"

	return [tokenizerHFile, tokenizerCFile, parsemapHFile, parsemapCFile]


def createBundle(name, unsortedList):
	enumTxt = "typedef enum {\n"
	switchMap = {}
	
	parsingArray = generateParsingArray(unsortedList)
	sortedList = parsingArray[0]

	for item in sortedList:
		if len(item) > 0:	
			if(item[0] == ":"):
				enumTxt += "\t" + name.upper() + "_" + item[1:].upper() + ",\n"
			else:
				enumTxt += "\t" + name.upper() + "_" + item.upper() + ",\n"
				insertItem(switchMap, item)
	enumTxt += "} " + name.upper() + "Type;\n\n"

	return [enumTxt, switchMap]

def generateParsingArray(unsortedList):
	mappedTokens = []
	unmappedTokens = []
	mappedTokenEntryStrings = []
	predeclarations = ""
	
	for item in unsortedList:
		potentialMatch = keywordLineMatcher.findall(item)[0]
		if potentialMatch[0] != '' and potentialMatch[1] != '':
			mappedTokens.append(potentialMatch[0])
			mappedTokenEntryStrings.append(potentialMatch)
		elif potentialMatch[0] != '':
			unmappedTokens.append(potentialMatch[0])
		else:
			print("Formatting Error, Skipping: " + item)


	parsingArray = generateEntries(mappedTokenEntryStrings)
	parsingHFile = "#include \"compiler_defs.h\"\n"
	parsingHFile += "extern ParseRule rules[];\n"
	parsingHFile += "#define NUM_PARSE_RULES " + str(len(mappedTokenEntryStrings)) + "\n"
	 
	return [mappedTokens + unmappedTokens, parsingHFile, parsingArray]

def generateEntries(matchArray):
	entries = ""
	predeclarations = ""
	usedPredeclarations = {}

	for match in matchArray:
		idText = match[0].replace(":", "")
		entryText = match[1]

		entryItems = entryText[1:-1].split(",")
		entry = "\t{"

		prefixFn = entryItems[0].strip()

		if prefixFn == '': 
			prefixFn = "NULL"
		elif prefixFn not in usedPredeclarations:
			predeclarations += "void " + prefixFn + "(bool canAssign);\n"
			usedPredeclarations[prefixFn] = None

		infixFn = entryItems[1].strip()

		if infixFn == '': 
			infixFn = "NULL"
		elif infixFn not in usedPredeclarations:
			predeclarations += "void " + infixFn + "(bool canAssign);\n"
			usedPredeclarations[infixFn] = None

		precedence = entryItems[2].strip().upper()
		if precedence == '': precedence = "NONE"
		precedence = "PC_" + precedence

		entries += "\t{" + prefixFn + ", " + infixFn + ", " + precedence + "},\t\t//TK_" + idText.upper() + "\n"
	return predeclarations + '\n\nParseRule rules[] = {\n' + entries + "};"

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
	functionBody += "\t\tdefault: return " + returnType.upper() + "_" + defaultReturn.upper() + ";\n"
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
		
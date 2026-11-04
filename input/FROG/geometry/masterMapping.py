#!/usr/bin/python3
import os
import sys
import itertools

def read5202Mapping(map5202: str):
	fileIn = open(map5202)
	lines = fileIn.readlines()
	mapping = {}
	for i in range(len(lines)):
		tokenize = lines[i].replace(' ', '').replace('\t', '').replace('\n', '')
		tokens = tokenize.split(',')
		str5202 = str(tokens[0])
		pinNum = int(tokens[1])
		mapping[str5202] = pinNum

	return mapping

def readBarrelMap(map5202: dict, barrelMap: str):
	fileIn = open(barrelMap)
	lines = fileIn.readlines()
	mapping = {}
	for i in range(1, len(lines)):
		tokenize = lines[i].replace(' ','').replace('\t', '').replace('\n', '')
		tokens = tokenize.split(',')
		barrel = int(tokens[0])
		board5202 = int(tokens[1])
		SiPMBoard = int(tokens[2])
		SiPMIndex = int(tokens[3])
		channel5202 = tokens[4]
		
		sipmName = f"{barrel}.{SiPMBoard}.{SiPMIndex}"
		boardMap = f"{board5202}.{map5202[channel5202]}"
		mapping[boardMap] = sipmName

	return mapping;


def main():
	mapping = read5202Mapping("./barrel-mapping/A5202_Mapping.txt")
	barrel7map = readBarrelMap(mapping, "./barrel-mapping/Barrel7_Mapping.txt")
	barrel6map = readBarrelMap(mapping, "./barrel-mapping/Barrel6_Mapping.txt")
	barrel5map = readBarrelMap(mapping, "./barrel-mapping/Barrel5_Mapping.txt")
	barrel4map = readBarrelMap(mapping, "./barrel-mapping/Barrel4_Mapping.txt")
	barrel3map = readBarrelMap(mapping, "./barrel-mapping/Barrel3_Mapping.txt")
	barrel2map = readBarrelMap(mapping, "./barrel-mapping/Barrel2_Mapping.txt")
	barrelMaps = [barrel7map, barrel6map, barrel5map, barrel4map, barrel3map, barrel2map]


	openFile = open("masterMapping.txt", "w")	# overwrite, never append
	openFile.write("Board.Channel, Barrel.Board.SiPM\n")
	for key, value in itertools.chain(barrel7map.items(), barrel6map.items(), barrel5map.items(), barrel4map.items(), barrel3map.items(), barrel2map.items()):
		openFile.write(f"{key},{value}\n")
	openFile.close()
	print("Wrote masterMapping.txt")


if __name__ == "__main__":
	main()

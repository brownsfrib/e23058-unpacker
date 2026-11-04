#include "TreeWriter.h"

#include <iostream>

TreeWriter::TreeWriter(const std::string &path, const char *treeName)
	: fFile(nullptr), fTree(nullptr), fEventNumber(0), fTimestamp(0){
	fFile = TFile::Open(path.c_str(), "RECREATE");
	if (!IsOpen()){
		std::cerr << "\033[1;31mTreeWriter: cannot create " << path << "\033[0m\n";
		return;
	}
	fTree = new TTree(treeName, "E23058 unpacked events");	// owned by fFile
	fTree->Branch("eventNumber", &fEventNumber, "eventNumber/L");
	fTree->Branch("timestamp",   &fTimestamp,   "timestamp/l");
}

TreeWriter::~TreeWriter(){
	Close();
}

void TreeWriter::Fill(ULong64_t timestamp){
	fTimestamp = timestamp;
	fTree->Fill();
	fEventNumber++;
}

void TreeWriter::Close(){
	if (!fFile){
		return;
	}
	fFile->cd();
	fTree->Write();
	fFile->Close();
	delete fFile;
	fFile = nullptr;
	fTree = nullptr;
}

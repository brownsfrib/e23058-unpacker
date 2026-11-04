#ifndef TREEWRITER_H
#define TREEWRITER_H

// C/C++ includes
#include <string>

// ROOT includes
#include "TFile.h"
#include "TTree.h"

/**
 * !\brief Writes the E23058 tree: one branch per data object, plus
 *         eventNumber and timestamp.
 *
 *   TreeWriter w("output/run_0100.root");
 *   w.Add("FROG.", &frogData);
 *   ...  per event:  w.Fill(eventTimestamp);
 *   w.Close();
 */
class TreeWriter{
	public:
		explicit TreeWriter(const std::string &path, const char *treeName = "E23058");
		~TreeWriter();

		bool IsOpen() const { return fFile && !fFile->IsZombie(); }

		// the object must stay alive (same address) until Close()
		template<class T> void Add(const char *branch, T *data){
			fTree->Branch(branch, data, 32000, 99);
		}

		void Fill(ULong64_t timestamp);
		void Close();

		Long64_t Entries() const { return fEventNumber; }

	private:
		TFile    *fFile;
		TTree    *fTree;
		Long64_t  fEventNumber;
		ULong64_t fTimestamp;
};

#endif // TREEWRITER_H

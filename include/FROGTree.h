#ifndef FROGTREE_H
#define FROGTREE_H

// ROOT includes
#include "TTree.h"

// Unpacker includes
#include "FROG.h"

/**
 * !\brief Writes the FROG TTree (one entry per kept event).
 *
 * Construct AFTER TFile::Open so the tree belongs to that file.
 * Every tree writer gets Fill() called for every kept event, so the trees
 * stay entry-aligned and can be combined with AddFriend().
 */
class FROGTree{
	public:
		static const int kMaxHits = 512;	// 6 boards x 64 channels = 384

		FROGTree(FROG &frog, const char *name = "FROG");

		double totCut = 10;	// keep channels with ToT >= totCut; < 0 keeps all FROG hits

		bool Build();		// copy this event into the buffers; true if anything to write
		void Fill(Long64_t eventNumber, ULong64_t timestamp);
		TTree *Tree() { return fTree; }

	private:
		FROG  &fFrog;
		TTree *fTree;

		Long64_t  fEventNumber;
		ULong64_t fTimestamp;
		Int_t     fNBoards;
		Int_t     fMult;
		Int_t     fBoard[kMaxHits];
		Int_t     fChannel[kMaxHits];
		Double_t  fLG[kMaxHits];
		Double_t  fHG[kMaxHits];
		Double_t  fToA[kMaxHits];
		Double_t  fToT[kMaxHits];
		Double_t  fTheta[kMaxHits];
		Double_t  fDTheta[kMaxHits];
		Double_t  fPhi[kMaxHits];
		Double_t  fDPhi[kMaxHits];
};

#endif // FROGTREE_H

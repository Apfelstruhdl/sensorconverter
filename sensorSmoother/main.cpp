#include <iostream>
#include <math.h>

#include <TROOT.h>
#include <TFile.h>
#include <TTree.h>

#include <QQueue>
#include <QDebug>

float meanValue (QList<float> &list)
{
  Q_ASSERT(list.size() > 0);
  float mean = 0;
  int nans = 0;
  QList<float>::const_iterator it;
  for ( it=list.begin() ; it != list.end(); it++ ) {
      if(isnan(*it))
        nans++;
      else
        mean += *it;
  }
  return mean/(list.size()-nans);
}

int main(int argc, char** argv)
{
  gROOT->Reset();

//   Connect file generated in $ROOTSYS/test
  TFile f("testbeam.root", "update");
  f.SetCompressionLevel(0);

  TTree* tree = (TTree*)(f.Get("sensors"));
  tree->SetCacheSize(200e6);
  tree->SetCacheEntryRange(0,tree->GetEntries());
  tree->AddBranchToCache("*",kTRUE);
  tree->OptimizeBaskets();
  std::cout << "tree with nEntries = " << tree->GetEntries() << std::endl;

  TBranch *bnTime  = tree->GetBranch("time");
  TBranch *bnPressure = tree->GetBranch("TRD_PRESSURE");

  TBranch *b = tree->GetBranch("TRD_PRESSURE_SMOOTHED");
  tree->GetListOfBranches()->Remove(b);
  tree->Write("",TObject::kOverwrite);


  std::cout << "tree->GetBranch(time) with nEntries = " << bnTime->GetEntries() << std::endl;
  std::cout << "tree->GetBranch(TRD_PRESSURE) with nEntries = " << bnPressure->GetEntries() << std::endl;

  float time = 0;
  float pressure = 0;
  float pressureSmoothed = 0;

  TBranch *bnPressureSmoothed = tree->Branch("TRD_PRESSURE_SMOOTHED", &pressureSmoothed);

  bnTime->SetAddress(&time);
  bnPressure->SetAddress(&pressure);

  Int_t nEntries = tree->GetEntries();

  const Int_t bufferSize = 360; //seconds before and ahead

  tree->GetEntry(0);

  QQueue<float> bufferOld;
  bufferOld.reserve(bufferSize);
  QQueue<float> bufferFuture;
  bufferFuture.reserve(bufferSize);

  //fill future buffer
  for (Int_t i=1;i<bufferSize+1;i++) {
    tree->GetEntry(i);
    bufferFuture << pressure;
  }

  //now history and future buffers are filled

  std::cout << "history mean = " << meanValue(bufferOld) << std::endl;
  std::cout << "future mean = " << meanValue(bufferFuture) << std::endl;

  for (Int_t i=0;i<nEntries;i++) {
    tree->GetEntry(i);
    bufferOld.enqueue(pressure);
    pressureSmoothed = (meanValue(bufferOld) + meanValue(bufferFuture))/2.;

    bnPressureSmoothed->Fill();

    if(i % 1000 == 0) {
      std::cout << i << std::endl;
      std::cout << "history size = " << bufferOld.size() << std::endl;
      std::cout << "future size = " << bufferFuture.size() << std::endl;
      std::cout << "history mean = " << meanValue(bufferOld) << std::endl;
      std::cout << "future mean = " << meanValue(bufferFuture) << std::endl;
      std::cout << "time = " << time << " pressure = " << pressure << " pressureSmoothed = " << pressureSmoothed << std::endl;
    }

    if(bufferOld.size() >= bufferSize)
      bufferOld.dequeue();

    //should not read twice:
    if (i+bufferSize < nEntries) {
      tree->GetEntry(i+bufferSize);
      bufferFuture.enqueue(pressure);
    }
    bufferFuture.dequeue();
  }


  tree->Write("",TObject::kOverwrite);
}




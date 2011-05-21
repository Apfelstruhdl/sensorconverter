#include <iostream>
#include <math.h>

#include <TROOT.h>
#include <TFile.h>
#include <TTree.h>
#include <TGraph.h>
#include <TGraphSmooth.h>
#include <TH2D.h>
#include <TVirtualFFT.h>

#include <QQueue>
#include <QDebug>

float meanValue (const QList<float> &list)
{
  //qDebug() << list;
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
  Q_ASSERT(list.size() > nans);
  return mean/(list.size()-nans);
}

float medianValue (const QList<float> &list)
{
  QList<float> sortedList;
  //qDebug() << list;
  Q_ASSERT(list.size() > 0);
  QList<float>::const_iterator it;
  for ( it=list.begin() ; it != list.end(); it++ ) {
      if(!isnan(*it))
        sortedList << *it;
  }
  double ret = 0;
  if(sortedList.size() > 0) {
    qSort(sortedList);
    return sortedList.at(sortedList.size()/2);
  }
  else
    return sqrt(-1);
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
  TBranch *bnTemp  = tree->GetBranch("TRD_GAS_COLD_TEMP");

  TBranch *b = tree->GetBranch("TRD_PRESSURE_SMOOTHED");
  tree->GetListOfBranches()->Remove(b);
  tree->Write("",TObject::kOverwrite);


  std::cout << "tree->GetBranch(time) with nEntries = " << bnTime->GetEntries() << std::endl;
  std::cout << "tree->GetBranch(TRD_PRESSURE) with nEntries = " << bnPressure->GetEntries() << std::endl;

  float time = 0;
  float pressure = 0;
  float pressureSmoothed = 0;
  float temp = 0;

  TBranch *bnPressureSmoothed = tree->Branch("TRD_PRESSURE_SMOOTHED", &pressureSmoothed);

  bnTime->SetAddress(&time);
  bnPressure->SetAddress(&pressure);
  bnTemp->SetAddress(&temp);

  Int_t nEntries = tree->GetEntries();

  const Int_t bufferSize = 20*60; //seconds around actual time

  tree->GetEntry(0);

  QQueue<float> smoothBuffer;
  smoothBuffer.reserve(bufferSize);




  //save for analysis
  QVector<float> times;
  times.reserve( bnTime->GetEntries());
  QVector<float> pressures;
  pressures.reserve( bnTime->GetEntries());
  QVector<float> pressuresSmoothed;
  pressuresSmoothed.reserve( bnTime->GetEntries());

  //correlation temperature pressure
  TH2D* corrPlot = new TH2D("temperatur-pressure correlation","temperatur-pressure correlation;temperature trd:pressure/mbar",100,20,40,100,1000,1150);

  //fill future buffer
  for (Int_t i=1;i<bufferSize/2;i++) {
    tree->GetEntry(i);
    smoothBuffer << pressure;

    times << time;
    pressures << pressure;
    //corrPlot->Fill(temp,pressure);
  }

  std::cout << "buffer mean = " << medianValue(smoothBuffer) << std::endl;

  for (Int_t i=0;i<nEntries;i++) {
    //if buffer is filled, removed last item
    if(smoothBuffer.size() >= bufferSize || (i+bufferSize/2 > nEntries))
      smoothBuffer.dequeue();

    //calculated the smoothBuffers mean, used to decide if next value is added:
    pressureSmoothed = meanValue(smoothBuffer);

    //read next value in tree, which is appended at front of the buffer:
    if (i+bufferSize/2 < nEntries) {
      tree->GetEntry(i+bufferSize/2);
      times << time;
      pressures << pressure;

    }
    pressuresSmoothed << pressureSmoothed;

    corrPlot->Fill(temp,pressureSmoothed);

    if (false) {
      qDebug("pressureSmoothed = %f", pressureSmoothed);
      qDebug("abweichung = %f", fabs(pressureSmoothed-pressure) / pressureSmoothed) ;
    }

    if((isnan(pressure) || isnan(pressureSmoothed)  || (fabs(pressureSmoothed-pressure) / pressureSmoothed < 0.01)) && pressure < 2500) //only accept pressure
      smoothBuffer.enqueue(pressure);
    else
      smoothBuffer.enqueue(sqrt(-1));


    bnPressureSmoothed->Fill();

    if(i % 1000 == 0) {
      std::cout << i << std::endl;
      std::cout << "buffer size = " << smoothBuffer.size() << std::endl;
      std::cout << "buffer mean = " << pressureSmoothed << std::endl;
      std::cout << "time = " << time << " pressure = " << pressure << " pressureSmoothed = " << pressureSmoothed << std::endl;
    }
  }

  tree->Write("",TObject::kOverwrite);



  //other stuff

  TGraph* pressureGraph = new TGraph(times.size(), &times[0], &pressures[0]);
  pressureGraph->SaveAs("pressureGraph.root");

  TGraph* pressureSmoothedGraph = new TGraph(times.size(), &times[0], &pressuresSmoothed[0]);
  pressureSmoothedGraph->SaveAs("pressureSmoothedGraph.root");

  corrPlot->SaveAs("corrPlot.root");

  //TGrapgSmoother
  TGraphSmooth* gs = new TGraphSmooth("normal");
  TGraph* grout = gs->SmoothSuper(pressureGraph,"",8);
  grout->SaveAs("smoothedGraph.root");

  //fftw
  /*
  Int_t n = pressures.size();
  Double_t *in = new Double_t[2*(n/2+1)];
  Double_t re_2,im_2;
  for (Int_t i=0; i<n; i++){
     in[i] = pressures.at(i);
  }

  TVirtualFFT *fft_own = TVirtualFFT::FFT(1, &n, "R2C ES K");
   if (!fft_own)
     return;
   fft_own->SetPoints(in);
   fft_own->Transform();
   //Copy all the output points:
   fft_own->GetPoints(in);
   */
}




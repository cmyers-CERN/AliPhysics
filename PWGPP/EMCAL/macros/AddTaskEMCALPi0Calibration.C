/// \file AddTaskEMCALPi0Calibration.C
/// \ingroup EMCALPerfAddTaskMacros
/// \brief Configuration of task AliAnalysisTaskEMCALPi0CalibSelection.
///
/// Configuration of task AliAnalysisTaskEMCALPi0CalibSelection, which fills invariant mass
/// histograms for each of the EMCal channels. It has to be executed in several iterations.
///
/// \author : Gustavo Conesa Balbastre <Gustavo.Conesa.Balbastre@cern.ch>, (LPSC-CNRS)
///

// Root6
#if !defined(__CINT__) || defined(__MAKECINT__)

#include <TString.h>
#include <TROOT.h>
#include <TSystem.h>

#include "AliLog.h"
#include "AliAnalysisTaskEMCALPi0CalibSelection.h"
#include "AliAnalysisManager.h"
#include "AliAnalysisDataContainer.h"

R__ADD_INCLUDE_PATH($ALICE_PHYSICS)
#include "PWGPP/EMCAL/macros/ConfigureEMCALRecoUtils.C"

#endif // CINT

/// Main method
///
/// The parameters for the analysis are:
/// \param calibPath : TString with full path and name of file with calibration factors from previous iteration.
/// \param trigger   : TString, event that triggered must contain this string. Leave for backward compatibility with old wagons
/// \param recalE    : Bool, recalibrate EMCal energy
/// \param recalT    : Bool, recalibrate EMCal time
/// \param rmBad     : Bool, remove bad channels
/// \param nonLin    : Bool, correct cluster non linearity
/// \param simu      : Bool, simulation or data.
/// \param outputFile: TString with name of output file (AnalysisResults.root).
/// \param trigSuffix :  A string with the trigger class, abbreviated, to run multiple triggers in same train
///
AliAnalysisTaskEMCALPi0CalibSelection * AddTaskEMCALPi0Calibration
(TString calibPath = "", // "alienpath/RecalibrationFactors.root"
 TString trigger   = "",
 Bool_t  recalE    = kFALSE, 
 Bool_t  recalT    = kFALSE,
 Bool_t  rmBad     = kFALSE,
 Bool_t  nonlin    = kTRUE,
 Bool_t  simu      = kFALSE,
 TString outputFile = "", // AnalysisResults.root
 const char *trigSuffix = ""
) 
{
  // Get the pointer to the existing analysis manager via the static access method.
  //==============================================================================
  AliAnalysisManager *mgr = AliAnalysisManager::GetAnalysisManager();
  if (!mgr) 
  {
    ::Error("AddTaskEMCALTriggerQA", "No analysis manager to connect to.");
    return NULL;
  }  
  
  // Check the analysis type using the event handlers connected to the analysis manager.
  //==============================================================================
  if (!mgr->GetInputEventHandler()) 
  {
    ::Error("AddTaskEMCALPi0Calibration", "This task requires an input event handler");
    return NULL;
  }
  
  TString wagon = trigSuffix;
  if ( wagon.Length() > 0 ) trigger = wagon;
  
  AliAnalysisTaskEMCALPi0CalibSelection * pi0calib = new AliAnalysisTaskEMCALPi0CalibSelection(Form("EMCALPi0Calibration_%s",trigger.Data()));
  //pi0calib->SetDebugLevel(10); 
  //pi0calib->UseFilteredEventAsInput();
  pi0calib->SetClusterMinEnergy(0.7);
  pi0calib->SetClusterMaxEnergy(10.);
  pi0calib->SetClusterLambda0Cuts(0.1,0.5);
  
  pi0calib->SetAsymmetryCut(1.);
  pi0calib->SetClusterMinNCells(1);
  pi0calib->SetNCellsGroup(0);
  pi0calib->SwitchOnSameSM();
  
  // Timing cuts
  pi0calib->SetPairDTimeCut(100);   //  20 ns in Run1  
  pi0calib->SetClusterMinTime(300); // 560 ns in Run1
  pi0calib->SetClusterMaxTime(800); // 610 ns in Run1

  pi0calib->SetTriggerName(trigger);
  
  // Centrality selection
//  pi0calib->SwitchOnCentrality();
//  pi0calib->SetCentralityRange(50.,90.);
//  pi0calib->SetCentralityClass("V0M");
  
  //
  // Cluster recalculation, Reco Utils configuration
  //
  AliEMCALRecoUtils * reco = pi0calib->GetEMCALRecoUtils();
  
  // Root5
#if defined(__CINT__)

  gROOT->LoadMacro("$ALICE_PHYSICS/PWGPP/EMCAL/macros/ConfigureEMCALRecoUtils.C");

#endif
  
  ConfigureEMCALRecoUtils(reco,
                          simu,                             
                          kTRUE, // exotic
                          nonlin,
                          recalE, 
                          rmBad,
                          recalT); 
  
  reco->SetNumberOfCellsFromEMCALBorder(0); // Do not remove clusters in borders!
  
  // recalibrate energy and do corrections because of Temperature corrections 
  pi0calib->SwitchOnClusterCorrection();
  reco->SwitchOnRecalibration();
  reco->SwitchOnRunDepCorrection();
  
  //reco->Print("");
  
  //---------------------
  // Geometry alignment
  //---------------------
  
  pi0calib->SetGeometryName("EMCAL_COMPLETE12SMV1_DCAL_8SM");

  pi0calib->SwitchOnLoadOwnGeometryMatrices();
  
  //---------------------
  // Pass recalibration factors
  // Do it here or inside the task
  // If previous pass not available (first) avoid recalculate clusters
  //---------------------
  
  pi0calib->SetCalibrationFilePath(calibPath);
  
  if(calibPath != "" && recalE)
  {
    printf("AddTaskEMCALPi0Calibration - Get the energy calibration factors from: \n %s \n",calibPath.Data());
    pi0calib->InitEnergyCalibrationFactors();
  }
   
  if(!recalE)
  {
    // Do not calibrate anything
    // First iteration, just fill histograms, switch off recalculation
    reco->SwitchOffRecalibration();
    reco->SwitchOffRunDepCorrection(); // Careful!!!, activate when T corrections are available.
    pi0calib->SwitchOffLoadOwnGeometryMatrices();
    pi0calib->SwitchOffRecalculatePosition();
    printf("AddTaskEMCALPi0Calibration - Pi0 Calibration: Do not recalculate the clusters! First iteration. \n");
    // check if time is corrected in case of calibration available!!!
  }
  
  pi0calib->PrintInfo();
  
  mgr->AddTask(pi0calib);
  
  if(outputFile.Length()==0) outputFile = AliAnalysisManager::GetCommonFileName(); 

  AliAnalysisDataContainer *cinput1 = mgr->GetCommonInputContainer();
                                                                                                
                                                                                                
  AliAnalysisDataContainer *coutput = 0; 
  if( wagon.Length()==0 )
  {
    coutput = mgr->CreateContainer(Form("Pi0Calibration_Trig%s",trigger.Data()), TList::Class(), 
                                   AliAnalysisManager::kOutputContainer,outputFile.Data());
  }
  else
  {
    TString containerName = "Pi0Calibration";
    coutput = mgr->CreateContainer(wagon, TList::Class(), 
                                   AliAnalysisManager::kOutputContainer,Form("%s:%s",outputFile.Data(),containerName.Data()));
  }  
    
//  AliAnalysisDataContainer *cout_cuts = mgr->CreateContainer(Form("ParamsPi0Calibration_Trig%s",trigger.Data()), 
//                                                             TList::Class(), AliAnalysisManager::kOutputContainer, 
//                                                             "AnalysisParameters.root");
  
  mgr->AddTask(pi0calib);
                                                             
  mgr->ConnectInput  (pi0calib, 0, cinput1);
  mgr->ConnectOutput (pi0calib, 1, coutput);
//  mgr->ConnectOutput (pi0calib, 2, cout_cuts);

  return pi0calib;
}

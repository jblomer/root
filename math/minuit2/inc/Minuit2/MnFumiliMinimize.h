// @(#)root/minuit2:$Id$
// Authors: M. Winkler, F. James, L. Moneta, A. Zsenei   2003-2005

/**********************************************************************
 *                                                                    *
 * Copyright (c) 2005 LCG ROOT Math team,  CERN/PH-SFT                *
 *                                                                    *
 **********************************************************************/

#ifndef ROOT_Minuit2_MnFumiliMinimize
#define ROOT_Minuit2_MnFumiliMinimize

#include "Minuit2/MnApplication.h"
#include "Minuit2/FumiliMinimizer.h"
#include "Minuit2/FumiliFCNBase.h"

#include <vector>

namespace ROOT {

namespace Minuit2 {

// class FumiliFCNBase;
// class FCNBase;

//___________________________________________________________________________
/**


API class for minimization using Fumili technology;
allows for user interaction: set/change parameters, do minimization,
change parameters, re-do minimization etc.;
also used by MnMinos and MnContours;


 */

class MnFumiliMinimize : public MnApplication {

public:
   /// construct from FumiliFCNBase + MnUserParameterState + MnStrategy
   MnFumiliMinimize(const FumiliFCNBase &fcn, const MnUserParameterState &par, const MnStrategy &str = MnStrategy{1})
      : MnApplication(fcn, MnUserParameterState(par), str), fMinimizer(FumiliMinimizer()), fFCN(fcn)
   {
   }

   MnFumiliMinimize(const MnFumiliMinimize &migr)
      : MnApplication(migr.Fcnbase(), migr.State(), migr.Strategy(), migr.NumOfCalls()), fMinimizer(migr.fMinimizer),
        fFCN(migr.Fcnbase())
   {
   }

   FumiliMinimizer &Minimizer() override { return fMinimizer; }
   const FumiliMinimizer &Minimizer() const override { return fMinimizer; }

   const FumiliFCNBase &Fcnbase() const override { return fFCN; }

   /// overwrite Minimize to use FumiliFCNBase
   FunctionMinimum operator()(unsigned int = 0, double = 0.1) override;

private:
   FumiliMinimizer fMinimizer;
   const FumiliFCNBase &fFCN;

private:
   // forbidden assignment of migrad (const FumiliFCNBase& = )
   MnFumiliMinimize &operator=(const MnFumiliMinimize &) { return *this; }
};

} // namespace Minuit2

} // namespace ROOT

#endif // ROOT_Minuit2_MnFumiliMinimize

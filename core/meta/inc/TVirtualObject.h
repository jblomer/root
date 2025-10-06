// @(#)root/io:$Id$
// Author: Philippe Canal July, 2008

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
  *************************************************************************/

#ifndef ROOT_TVirtualObject
#define ROOT_TVirtualObject

#include "TClassRef.h"

/// \class TVirtualObject
/// \ingroup IO
///
/// Wrapper around an object and giving indirect access to its content
/// even if the object is not of a class in the Cint/Reflex dictionary.
///
/// The TVirtualObject represents the schema evolution class / staging area,
/// i.e. the description of the conversion between different class versions.
class TVirtualObject {
public:
   TClassRef  fClass;  ///< Corresponds to the conversion streamer info ("Class@@Version")
   void      *fObject; ///< The staging area of the on-disk class members required during I/O rule execution

   TVirtualObject(TClass *cl) : fClass(cl), fObject(cl ? cl->New() : nullptr) {}
   TVirtualObject(const TVirtualObject &) = delete;
   TVirtualObject(TVirtualObject &&) = delete;
   TVirtualObject &operator=(const TVirtualObject &) = delete;
   TVirtualObject &operator=(TVirtualObject &&) = delete;
   ~TVirtualObject()
   {
      if (fClass)
         fClass->Destructor(fObject);
   }

   TClass *GetClass() const { return fClass; }
   void   *GetObject() const { return fObject; }
};

#endif // ROOT_TVirtualObject

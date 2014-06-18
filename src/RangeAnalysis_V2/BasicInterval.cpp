/*
 * BasicInterval.cpp
 *
 * Copyright (C) 2011-2012  Victor Hugo Sperle Campos
 * Copyright (C) 2013-2014  Raphael Ernani Rodrigues
 */

#include "BasicInterval.h"

using namespace llvm;

// ========================================================================== //
// BasicInterval
// ========================================================================== //

BasicInterval::BasicInterval(const Range& range) :
		range(range) {
}

BasicInterval::BasicInterval() :
		range(Range()) {
}

BasicInterval::BasicInterval(const APInt& l, const APInt& u) :
		range(Range(l, u)) {
}

// This is a base class, its dtor must be virtual.
BasicInterval::~BasicInterval() {
}

/// Pretty print.
void BasicInterval::print(raw_ostream& OS) const {
	this->getRange().print(OS);
}

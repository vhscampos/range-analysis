/*
 * SymbolicInterval.h
 *
 * Copyright (C) 2011-2012  Victor Hugo Sperle Campos
 * Copyright (C) 2013-2014  Raphael Ernani Rodrigues
 */

#ifndef SYMBOLICINTERVAL_H_
#define SYMBOLICINTERVAL_H_

#include "llvm/IR/Instructions.h"

#include "BasicInterval.h"

using namespace llvm;

namespace llvm {
	/// This is an interval that contains a symbolic limit, which is
	/// given by the bounds of a program name, e.g.: [-inf, ub(b) + 1].
	class SymbInterval : public BasicInterval {
	private:
		// The bound. It is a node which limits the interval of this range.
		const Value* bound;
		// The predicate of the operation in which this interval takes part.
		// It is useful to know how we can constrain this interval
		// after we fix the intersects.
		CmpInst::Predicate pred;

	public:
		SymbInterval(const Range& range, const Value* bound, CmpInst::Predicate pred);
		~SymbInterval();
		// Methods for RTTI
		virtual IntervalId getValueId() const {return SymbIntervalId;}
		static bool classof(SymbInterval const *) {return true;}
		static bool classof(BasicInterval const *BI) {
			return BI->getValueId() == SymbIntervalId;
		}
		/// Returns the opcode of the operation that create this interval.
		CmpInst::Predicate getOperation() const {return this->pred;}
		/// Returns the node which is the bound of this interval.
		const Value* getBound() const {return this->bound;}
		/// Replace symbolic intervals with hard-wired constants.
		void fixIntersects(Range boundRange);
		/// Prints the content of the interval.
		void print(raw_ostream& OS) const;
	};
}

#endif /* SYMBOLICINTERVAL_H_ */

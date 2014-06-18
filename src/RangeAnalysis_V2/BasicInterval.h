/*
 * BasicInterval.h
 *
 * Copyright (C) 2011-2012  Victor Hugo Sperle Campos
 * Copyright (C) 2013-2014  Raphael Ernani Rodrigues
 */


#ifndef RA_BASICINTERVAL_H_
#define RA_BASICINTERVAL_H_


#include "llvm/IR/Value.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/raw_ostream.h"

#include "Range.h"

using namespace llvm;

namespace llvm {

	enum IntervalId {
		BasicIntervalId,
		SymbIntervalId
	};

	/// This class represents a basic interval of values. This class could inherit
	/// from LLVM's Range class, since it *is a Range*. However,
	/// LLVM's Range class doesn't have a virtual constructor =/.
	class BasicInterval {
	private:
		Range range;

	public:
		BasicInterval(const Range& range);
		BasicInterval(const APInt& l, const APInt& u);
		BasicInterval();
		virtual ~BasicInterval(); // This is a base class.
		// Methods for RTTI
		virtual IntervalId getValueId() const {return BasicIntervalId;}
		static bool classof(BasicInterval const *) {return true;}
		/// Returns the range of this interval.
		const Range& getRange() const {return this->range;}
		/// Sets the range of this interval to another range.
		void setRange(const Range& newRange) {
			this->range = newRange;

			// Check if lower bound is greater than upper bound. If it is,
			// set range to empty
			if (this->range.getLower().sgt(this->range.getUpper())) {
				this->range.setEmpty();
			}
		}
		/// Pretty print.
		virtual void print(raw_ostream& OS) const;
	};
}

#endif /* BASICINTERVAL_H_ */

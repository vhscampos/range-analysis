/*
 * ProgressVector.cpp
 *
 *  Created on: Jan 17, 2014
 *      Author: raphael
 */

#include "ProgressVector.h"

namespace llvm {

ProgressVector::ProgressVector(std::list<Value*> redefinition) {


	bool first = true;
	Value* PreviousValue = NULL;
	Expr PreviousExpr;
	Expr firstExpr;

	for(std::list<Value*>::iterator it = redefinition.begin();  it != redefinition.end(); it++){

		Value* CurrentValue = *it;

		Expr CurrentExpr;

		if (first) {
			firstExpr = Expr(CurrentValue);

			//errs() << "First Expr:" << firstExpr << "\n";

			vecExpr = firstExpr;
		} else {

			if (PHINode* PHI = dyn_cast<PHINode>(CurrentValue)) {
				//This will get the correct incoming value.
				CurrentValue = PreviousValue;
			}

			CurrentExpr = Expr(CurrentValue, 1);

			std::vector<std::pair<Expr, Expr> > vecSubs;
			vecSubs.push_back(std::pair<Expr, Expr>(PreviousExpr, vecExpr));
			CurrentExpr.subs(vecSubs);

			vecExpr = CurrentExpr;
		}

		PreviousExpr = Expr(CurrentValue);
		PreviousValue = CurrentValue;
		first = false;
	}

	//Here we compute the delta
	vecExpr = vecExpr - firstExpr;

}

Value* ProgressVector::getUniqueValue(Type* constType) {

	if (vecExpr.isNumber()) {

		return ConstantInt::get(constType, vecExpr.getNumber());
	}

	return vecExpr.getUniqueValue();
}

} /* namespace llvm */

bool ProgressVector::isConstant() {
	return vecExpr.isNumber();
}

int ProgressVector::getConstantValue() {

	assert(isConstant() && "Vector is not constant!");

	return vecExpr.getNumber();
}

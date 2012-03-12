#include "RangeAnalysisTest.h"

unsigned MAX_BIT_INT = 1;
APInt Min = APInt::getSignedMinValue(MAX_BIT_INT);
APInt Max = APInt::getSignedMaxValue(MAX_BIT_INT);
APInt Zero(MAX_BIT_INT, 0, true);

void RangeUnitTest::printStats() {
	errs() << "\n//********************** STATS *******************************//\n";
	errs() << "\tFailed: " << failed << " (" << failed / total;
	if (failed > 0)
		errs() << "." << int(100 * ((double)failed/total));
	errs() << "%)\n";
	errs() << "\tTotal: " << total << "\n";
	errs()
			<< "//************************************************************//\n";
}

#define NEW_APINT(n) APInt(MAX_BIT_INT,n,true)
#define NEW_RANGE(l,u) Range(l, u, Regular)
#define NEW_RANGE_N(l,u) NEW_RANGE(NEW_APINT(l),NEW_APINT(u))
#define NEW_RANGE_MAX(l) NEW_RANGE(NEW_APINT(l), Max)
#define NEW_RANGE_MIN(u) NEW_RANGE(Min, NEW_APINT(u))

bool RangeUnitTest::runOnModule(Module & M) {
	MAX_BIT_INT = InterProceduralRA<Cousot>::getMaxBitWidth(M);
	Min = APInt::getSignedMinValue(MAX_BIT_INT);
	Max = APInt::getSignedMaxValue(MAX_BIT_INT);
	Zero = APInt(MAX_BIT_INT, 0, true);

	errs() << "Running unit tests for Range class!\n";
	// --------------------------- Shared Objects -------------------------//
	Range unknown(Min, Max, Unknown);
	Range empty(Min, Max, Empty);
	Range zero(Zero, Zero);
	Range infy(Min, Max);
	Range pos(Zero, Max);
	Range neg(Min, Zero);
	Range smallNeg(NEW_APINT(-17), Zero);
	Range smallPos(Zero, NEW_APINT(35));
	Range avgNeg(NEW_APINT(-27817), Zero);
	Range avgPos(Zero, NEW_APINT(182935));
	Range bigNeg(NEW_APINT(-281239847), Zero);
	Range bigPos(Zero, NEW_APINT(211821935));

	// -------------------------------- ADD --------------------------------//
	// [a, b] - [c, d] = [a + c, b + d]
	ASSERT_TRUE("ADD1", add, infy, infy, infy);
	ASSERT_TRUE("ADD2", add, zero, infy, infy);
	ASSERT_TRUE("ADD3", add, zero, zero, zero);
	ASSERT_TRUE("ADD4", add, neg, zero, neg);
	ASSERT_TRUE("ADD5", add, neg, infy, infy);
	ASSERT_TRUE("ADD6", add, neg, neg, neg);
	ASSERT_TRUE("ADD7", add, pos, zero, pos);
	ASSERT_TRUE("ADD8", add, pos, infy, infy);
	ASSERT_TRUE("ADD9", add, pos, neg, infy);
	ASSERT_TRUE("ADD10", add, pos, pos, pos);
	ASSERT_TRUE("ADD11", add, smallNeg, infy, infy);
	ASSERT_TRUE("ADD12", add, smallNeg, zero, smallNeg);
	ASSERT_TRUE("ADD13", add, smallNeg, pos, NEW_RANGE_MAX(-17));
	ASSERT_TRUE("ADD14", add, smallNeg, neg, neg);
	ASSERT_TRUE("ADD15", add, smallNeg, smallNeg, NEW_RANGE_N(-34,0));
	ASSERT_TRUE("ADD16", add, smallNeg, smallPos, NEW_RANGE_N(-17,35));
	ASSERT_TRUE("ADD17", add, smallNeg, avgNeg, NEW_RANGE_N(-27834,0));
	ASSERT_TRUE("ADD18", add, smallNeg, avgPos, NEW_RANGE_N(-17,182935));
	ASSERT_TRUE("ADD19", add, smallNeg, bigNeg, NEW_RANGE_N(-281239864,0));
	ASSERT_TRUE("ADD20", add, smallNeg, bigPos, NEW_RANGE_N(-17, 211821935));
	ASSERT_TRUE("ADD21", add, smallPos, infy, infy);
	ASSERT_TRUE("ADD22", add, smallPos, zero, smallPos);
	ASSERT_TRUE("ADD23", add, smallPos, pos, pos);
	ASSERT_TRUE("ADD24", add, smallPos, neg, NEW_RANGE_MIN(35));
	ASSERT_TRUE("ADD25", add, smallPos, smallPos, NEW_RANGE_N(0,70));
	ASSERT_TRUE("ADD26", add, smallPos, avgNeg, NEW_RANGE_N(-27817, 35));
	ASSERT_TRUE("ADD27", add, smallPos, avgPos, NEW_RANGE_N(0, 182970));
	ASSERT_TRUE("ADD28", add, smallPos, bigNeg, NEW_RANGE_N(-281239847, 35));
	ASSERT_TRUE("ADD29", add, smallPos, bigPos, NEW_RANGE_N(0, 211821970));
	ASSERT_TRUE("ADD30", add, avgNeg, infy, infy);
	ASSERT_TRUE("ADD31", add, avgNeg, zero, avgNeg);
	ASSERT_TRUE("ADD32", add, avgNeg, pos, NEW_RANGE_MAX(-27817));
	ASSERT_TRUE("ADD33", add, avgNeg, neg, neg);
	ASSERT_TRUE("ADD34", add, avgNeg, avgNeg, NEW_RANGE_N(-55634, 0));
	ASSERT_TRUE("ADD35", add, avgNeg, avgPos, NEW_RANGE_N(-27817, 182935));
	ASSERT_TRUE("ADD36", add, avgNeg, bigNeg, NEW_RANGE_N(-281267664, 0));
	ASSERT_TRUE("ADD37", add, avgNeg, bigPos, NEW_RANGE_N(-27817, 211821935));
	ASSERT_TRUE("ADD38", add, avgPos, infy, infy);
	ASSERT_TRUE("ADD39", add, avgPos, zero, avgPos);
	ASSERT_TRUE("ADD40", add, avgPos, pos, pos);
	ASSERT_TRUE("ADD41", add, avgPos, neg, NEW_RANGE_MIN(182935));
	ASSERT_TRUE("ADD42", add, avgPos, avgPos, NEW_RANGE_N(0, 365870));
	ASSERT_TRUE("ADD43", add, avgPos, bigNeg, NEW_RANGE_N(-281239847, 182935));
	ASSERT_TRUE("ADD44", add, avgPos, bigPos, NEW_RANGE_N(0, 212004870));
	ASSERT_TRUE("ADD45", add, bigNeg, infy, infy);
	ASSERT_TRUE("ADD46", add, bigNeg, zero, bigNeg);
	ASSERT_TRUE("ADD47", add, bigNeg, pos, NEW_RANGE_MAX(-281239847));
	ASSERT_TRUE("ADD48", add, bigNeg, neg, neg);
	ASSERT_TRUE("ADD49", add, bigNeg, bigNeg, NEW_RANGE_N(-562479694, 0));
	ASSERT_TRUE("ADD50", add, bigNeg, bigPos, NEW_RANGE_N(-281239847, 211821935));
	ASSERT_TRUE("ADD51", add, bigPos, infy, infy);
	ASSERT_TRUE("ADD52", add, bigPos, zero, bigPos);
	ASSERT_TRUE("ADD53", add, bigPos, pos, pos);
	ASSERT_TRUE("ADD54", add, bigPos, neg, NEW_RANGE_MIN(211821935));
	ASSERT_TRUE("ADD55", add, bigPos, bigPos, NEW_RANGE_N(0,423643870));

	// -------------------------------- SUB --------------------------------//
	// [a, b] - [c, d] = [a - d, b - c]
	ASSERT_TRUE("SUB1", sub, infy, infy, infy);
	ASSERT_TRUE("SUB2", sub, infy, zero, infy);
	ASSERT_TRUE("SUB3", sub, infy, pos, infy);
	ASSERT_TRUE("SUB4", sub, infy, neg, infy);
	ASSERT_TRUE("SUB5", sub, zero, zero, zero);
	ASSERT_TRUE("SUB6", sub, zero, infy, infy);
	ASSERT_TRUE("SUB7", sub, zero, pos, neg);
	ASSERT_TRUE("SUB8", sub, zero, neg, pos);
	ASSERT_TRUE("SUB9", sub, pos, zero, pos);
	ASSERT_TRUE("SUB10", sub, pos, infy, infy);
	ASSERT_TRUE("SUB11", sub, pos, neg, pos);
	ASSERT_TRUE("SUB12", sub, pos, pos, infy);
	ASSERT_TRUE("SUB13", sub, neg, zero, neg);
	ASSERT_TRUE("SUB14", sub, neg, infy, infy);
	ASSERT_TRUE("SUB15", sub, neg, neg, infy);
	ASSERT_TRUE("SUB16", sub, neg, pos, neg);

	// -------------------------------- MUL --------------------------------//
	//  [a, b] * [c, d] = [Min(a*c, a*d, b*c, b*d), Max(a*c, a*d, b*c, b*d)]
	ASSERT_TRUE("MUL1", mul, infy, infy, infy);
	ASSERT_TRUE("MUL2", mul, zero, infy, infy);
	ASSERT_TRUE("MUL3", mul, zero, zero, zero);
	ASSERT_TRUE("MUL4", mul, neg, zero, zero);
	ASSERT_TRUE("MUL5", mul, neg, infy, infy);
	ASSERT_TRUE("MUL6", mul, neg, neg, pos);
	ASSERT_TRUE("MUL7", mul, pos, zero, zero);
	ASSERT_TRUE("MUL8", mul, pos, infy, infy);
	ASSERT_TRUE("MUL9", mul, pos, neg, neg);
	ASSERT_TRUE("MUL10", mul, pos, pos, pos);
	ASSERT_TRUE("MUL11", mul, smallNeg, infy, infy);
	ASSERT_TRUE("MUL12", mul, smallNeg, zero, zero);
	ASSERT_TRUE("MUL13", mul, smallNeg, pos, neg);
	ASSERT_TRUE("MUL14", mul, smallNeg, neg, pos);
	ASSERT_TRUE("MUL15", mul, smallNeg, smallNeg, NEW_RANGE_N(0, 289));
	ASSERT_TRUE("MUL16", mul, smallNeg, smallPos, NEW_RANGE_N(-595, 0));
	ASSERT_TRUE("MUL17", mul, smallNeg, avgNeg, NEW_RANGE_N(0, 472889));
	ASSERT_TRUE("MUL18", mul, smallNeg, avgPos, NEW_RANGE_N(-3109895, 0));
	ASSERT_TRUE("MUL19", mul, smallNeg, bigNeg, NEW_RANGE_N(0, 486110103));
	ASSERT_TRUE("MUL20", mul, smallNeg, bigPos, NEW_RANGE_N(0, 693994401));
	ASSERT_TRUE("MUL21", mul, smallPos, infy, infy);
	ASSERT_TRUE("MUL22", mul, smallPos, zero, zero);
	ASSERT_TRUE("MUL23", mul, smallPos, pos, pos);
	ASSERT_TRUE("MUL24", mul, smallPos, neg, neg);
	ASSERT_TRUE("MUL25", mul, smallPos, smallPos, NEW_RANGE_N(0, 1225));
	ASSERT_TRUE("MUL26", mul, smallPos, avgNeg, NEW_RANGE_N(-973595, 0));
	ASSERT_TRUE("MUL27", mul, smallPos, avgPos, NEW_RANGE_N(0, 6402725));
	ASSERT_TRUE("MUL28", mul, smallPos, bigNeg, NEW_RANGE_N(-1253460053, 0));
	ASSERT_TRUE("MUL29", mul, smallPos, bigPos, NEW_RANGE_N(-1176166867, 0));
	ASSERT_TRUE("MUL30", mul, avgNeg, infy, infy);
	ASSERT_TRUE("MUL31", mul, avgNeg, zero, zero);
	ASSERT_TRUE("MUL32", mul, avgNeg, pos, neg);
	ASSERT_TRUE("MUL33", mul, avgNeg, neg, pos);
	ASSERT_TRUE("MUL34", mul, avgNeg, avgNeg, NEW_RANGE_N(0, 773785489));
	ASSERT_TRUE("MUL35", mul, avgNeg, avgPos, NEW_RANGE_N(-793735599, 0));
	ASSERT_TRUE("MUL38", mul, avgPos, infy, infy);
	ASSERT_TRUE("MUL39", mul, avgPos, zero, zero);
	ASSERT_TRUE("MUL40", mul, avgPos, pos, pos);
	ASSERT_TRUE("MUL41", mul, avgPos, neg, neg);
	ASSERT_TRUE("MUL45", mul, bigNeg, infy, infy);
	ASSERT_TRUE("MUL46", mul, bigNeg, zero, zero);
	ASSERT_TRUE("MUL47", mul, bigNeg, pos, neg);
	ASSERT_TRUE("MUL48", mul, bigNeg, neg, pos);
	ASSERT_TRUE("MUL51", mul, bigPos, infy, infy);
	ASSERT_TRUE("MUL52", mul, bigPos, zero, zero);
	ASSERT_TRUE("MUL53", mul, bigPos, pos, pos);
	ASSERT_TRUE("MUL54", mul, bigPos, neg, neg);

	// -------------------------------- DIV --------------------------------//
	// [a, b] / [c, d] = [Min(a/c, a/d, b/c, b/d), Max(a/c, a/d, b/c, b/d)]
	ASSERT_TRUE("UDIV1", udiv, infy, infy, unknown);
	ASSERT_TRUE("UDIV2", udiv, infy, zero, unknown);
	ASSERT_TRUE("UDIV3", udiv, infy, pos, unknown);
	ASSERT_TRUE("UDIV4", udiv, infy, neg, unknown);
	ASSERT_TRUE("UDIV5", udiv, infy, smallNeg, unknown);
	ASSERT_TRUE("UDIV6", udiv, infy, smallPos, unknown);
	ASSERT_TRUE("UDIV7", udiv, infy, avgNeg, unknown);
	ASSERT_TRUE("UDIV8", udiv, infy, avgPos, unknown);
	ASSERT_TRUE("UDIV9", udiv, infy, bigNeg, unknown);
	ASSERT_TRUE("UDIV10", udiv, infy, bigPos, unknown);
	ASSERT_TRUE("UDIV11", udiv, zero, zero, unknown);
	ASSERT_TRUE("UDIV12", udiv, zero, infy, zero);
	ASSERT_TRUE("UDIV13", udiv, zero, pos, unknown);
	ASSERT_TRUE("UDIV14", udiv, zero, neg, unknown);
	ASSERT_TRUE("UDIV15", udiv, zero, smallNeg, unknown);
	ASSERT_TRUE("UDIV16", udiv, zero, smallPos, unknown);
	ASSERT_TRUE("UDIV17", udiv, zero, avgNeg, unknown);
	ASSERT_TRUE("UDIV18", udiv, zero, avgPos, unknown);
	ASSERT_TRUE("UDIV19", udiv, zero, bigNeg, unknown);
	ASSERT_TRUE("UDIV20", udiv, zero, bigPos, unknown);
	ASSERT_TRUE("UDIV21", udiv, pos, zero, unknown);
	ASSERT_TRUE("UDIV22", udiv, pos, infy, unknown);
	ASSERT_TRUE("UDIV23", udiv, pos, neg, unknown);
	ASSERT_TRUE("UDIV24", udiv, pos, pos, unknown);
	ASSERT_TRUE("UDIV25", udiv, pos, smallNeg, unknown);
	ASSERT_TRUE("UDIV26", udiv, pos, smallPos, unknown);
	ASSERT_TRUE("UDIV27", udiv, pos, avgNeg, unknown);
	ASSERT_TRUE("UDIV28", udiv, pos, avgPos, unknown);
	ASSERT_TRUE("UDIV29", udiv, pos, bigNeg, unknown);
	ASSERT_TRUE("UDIV30", udiv, pos, bigPos, unknown);
	ASSERT_TRUE("UDIV31", udiv, neg, zero, unknown);
	ASSERT_TRUE("UDIV32", udiv, neg, infy, unknown);
	ASSERT_TRUE("UDIV33", udiv, neg, neg, unknown);
	ASSERT_TRUE("UDIV34", udiv, neg, pos, unknown);
	ASSERT_TRUE("UDIV35", udiv, neg, smallNeg, unknown);
	ASSERT_TRUE("UDIV36", udiv, neg, smallPos, unknown);
	ASSERT_TRUE("UDIV37", udiv, neg, avgNeg, unknown);
	ASSERT_TRUE("UDIV38", udiv, neg, avgPos, unknown);
	ASSERT_TRUE("UDIV39", udiv, neg, bigNeg, unknown);
	ASSERT_TRUE("UDIV40", udiv, neg, bigPos, unknown);
	ASSERT_TRUE("UDIV41", udiv, smallNeg, infy, unknown);
	ASSERT_TRUE("UDIV42", udiv, smallNeg, zero, unknown);
	ASSERT_TRUE("UDIV43", udiv, smallNeg, pos, unknown);
	ASSERT_TRUE("UDIV44", udiv, smallNeg, neg, unknown);
	ASSERT_TRUE("UDIV45", udiv, smallNeg, smallNeg, unknown);
	ASSERT_TRUE("UDIV46", udiv, smallNeg, smallPos, unknown);
	ASSERT_TRUE("UDIV47", udiv, smallNeg, avgNeg, unknown);
	ASSERT_TRUE("UDIV48", udiv, smallNeg, avgPos, unknown);
	ASSERT_TRUE("UDIV49", udiv, smallNeg, bigNeg, unknown);
	ASSERT_TRUE("UDIV50", udiv, smallNeg, bigPos, unknown);
	ASSERT_TRUE("UDIV51", udiv, smallPos, infy, unknown);
	ASSERT_TRUE("UDIV52", udiv, smallPos, zero, unknown);
	ASSERT_TRUE("UDIV53", udiv, smallPos, pos, unknown);
	ASSERT_TRUE("UDIV54", udiv, smallPos, neg, unknown);
	ASSERT_TRUE("UDIV55", udiv, smallPos, smallNeg, unknown);
	ASSERT_TRUE("UDIV56", udiv, smallPos, smallPos, unknown);
	ASSERT_TRUE("UDIV57", udiv, smallPos, avgNeg, unknown);
	ASSERT_TRUE("UDIV58", udiv, smallPos, avgPos, unknown);
	ASSERT_TRUE("UDIV59", udiv, smallPos, bigNeg, unknown);
	ASSERT_TRUE("UDIV60", udiv, smallPos, bigPos, unknown);
	ASSERT_TRUE("UDIV61", udiv, avgNeg, infy, unknown);
	ASSERT_TRUE("UDIV62", udiv, avgNeg, zero, unknown);
	ASSERT_TRUE("UDIV63", udiv, avgNeg, pos, unknown);
	ASSERT_TRUE("UDIV64", udiv, avgNeg, neg, unknown);
	ASSERT_TRUE("UDIV65", udiv, avgNeg, smallNeg, unknown);
	ASSERT_TRUE("UDIV66", udiv, avgNeg, smallPos, unknown);
	ASSERT_TRUE("UDIV67", udiv, avgNeg, avgNeg, unknown);
	ASSERT_TRUE("UDIV68", udiv, avgNeg, avgPos, unknown);
	ASSERT_TRUE("UDIV69", udiv, avgNeg, bigNeg, unknown);
	ASSERT_TRUE("UDIV70", udiv, avgNeg, bigPos, unknown);
	ASSERT_TRUE("UDIV71", udiv, avgPos, infy, unknown);
	ASSERT_TRUE("UDIV72", udiv, avgPos, zero, unknown);
	ASSERT_TRUE("UDIV73", udiv, avgPos, pos, unknown);
	ASSERT_TRUE("UDIV74", udiv, avgPos, neg, unknown);
	ASSERT_TRUE("UDIV75", udiv, avgPos, smallNeg, unknown);
	ASSERT_TRUE("UDIV76", udiv, avgPos, smallPos, unknown);
	ASSERT_TRUE("UDIV77", udiv, avgPos, avgNeg, unknown);
	ASSERT_TRUE("UDIV78", udiv, avgPos, avgPos, unknown);
	ASSERT_TRUE("UDIV79", udiv, avgPos, bigNeg, unknown);
	ASSERT_TRUE("UDIV80", udiv, avgPos, bigPos, unknown);
	ASSERT_TRUE("UDIV81", udiv, bigNeg, infy, unknown);
	ASSERT_TRUE("UDIV82", udiv, bigNeg, zero, unknown);
	ASSERT_TRUE("UDIV83", udiv, bigNeg, pos, unknown);
	ASSERT_TRUE("UDIV84", udiv, bigNeg, neg, unknown);
	ASSERT_TRUE("UDIV85", udiv, bigNeg, smallNeg, unknown);
	ASSERT_TRUE("UDIV86", udiv, bigNeg, smallPos, unknown);
	ASSERT_TRUE("UDIV87", udiv, bigNeg, avgNeg, unknown);
	ASSERT_TRUE("UDIV88", udiv, bigNeg, avgPos, unknown);
	ASSERT_TRUE("UDIV89", udiv, bigNeg, bigNeg, unknown);
	ASSERT_TRUE("UDIV90", udiv, bigNeg, bigPos, unknown);
	ASSERT_TRUE("UDIV91", udiv, bigPos, infy, unknown);
	ASSERT_TRUE("UDIV92", udiv, bigPos, zero, unknown);
	ASSERT_TRUE("UDIV93", udiv, bigPos, pos, unknown);
	ASSERT_TRUE("UDIV94", udiv, bigPos, neg, unknown);
	ASSERT_TRUE("UDIV95", udiv, bigPos, smallNeg, unknown);
	ASSERT_TRUE("UDIV96", udiv, bigPos, smallPos, unknown);
	ASSERT_TRUE("UDIV97", udiv, bigPos, avgNeg, unknown);
	ASSERT_TRUE("UDIV98", udiv, bigPos, avgPos, unknown);
	ASSERT_TRUE("UDIV99", udiv, bigPos, bigNeg, unknown);
	ASSERT_TRUE("UDIV100", udiv, bigPos, bigPos, unknown);

	printStats();
	return true;
}

char RangeUnitTest::ID = 3;
static RegisterPass<RangeUnitTest> T("ra-test-range",
		"Run unit test for class Range");

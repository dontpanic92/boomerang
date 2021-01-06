#pragma region License
/*
 * This file is part of the Boomerang Decompiler.
 *
 * See the file "LICENSE.TERMS" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 */
#pragma endregion License
#include "StatementTest.h"


#include "boomerang/core/Settings.h"
#include "boomerang/db/proc/ProcCFG.h"
#include "boomerang/db/module/Module.h"
#include "boomerang/db/signature/Signature.h"
#include "boomerang/db/BasicBlock.h"
#include "boomerang/db/LowLevelCFG.h"
#include "boomerang/db/Prog.h"
#include "boomerang/db/proc/UserProc.h"
#include "boomerang/decomp/ProcDecompiler.h"
#include "boomerang/decomp/ProgDecompiler.h"
#include "boomerang/ssl/exp/Const.h"
#include "boomerang/ssl/exp/Location.h"
#include "boomerang/ssl/exp/RefExp.h"
#include "boomerang/ssl/exp/Terminal.h"
#include "boomerang/ssl/exp/Ternary.h"
#include "boomerang/ssl/statements/Assign.h"
#include "boomerang/ssl/statements/ImplicitAssign.h"
#include "boomerang/ssl/statements/CallStatement.h"
#include "boomerang/ssl/statements/CaseStatement.h"
#include "boomerang/ssl/statements/BranchStatement.h"
#include "boomerang/ssl/statements/BoolAssign.h"
#include "boomerang/ssl/statements/PhiAssign.h"
#include "boomerang/ssl/statements/ReturnStatement.h"
#include "boomerang/ssl/RTL.h"
#include "boomerang/ssl/type/ArrayType.h"
#include "boomerang/ssl/type/IntegerType.h"
#include "boomerang/passes/PassManager.h"
#include "boomerang/util/log/Log.h"

#include <sstream>
#include <map>


#define HELLO_X86      getFullSamplePath("x86/hello")
#define GLOBAL1_X86    getFullSamplePath("x86/global1")


#define TEST_PROP(name, exp, canProp) \
    do { \
        QTest::newRow(name) << SharedExpWrapper(exp) << canProp; \
    } while (false)


SharedExp makeFlagCallArgs() { return Terminal::get(opNil); }

template<typename Arg, typename... Args>
SharedExp makeFlagCallArgs(Arg arg, Args... args) { return Binary::get(opList, arg, makeFlagCallArgs(args...)); }

template<typename... Args>
SharedExp makeFlagCall(const QString &name, Args... args) { return Binary::get(opFlagCall, Const::get(name), makeFlagCallArgs(args...)); }



void StatementTest::testFragment()
{
    Prog prog("testProg", &m_project);
    BasicBlock *bb = prog.getCFG()->createBB(BBType::Oneway, createInsns(Address(0x1000), 1));

    UserProc proc(Address(0x1000), "test", nullptr);
    IRFragment *frag = proc.getCFG()->createFragment(FragType::Oneway, createRTLs(Address(0x1000), 1, 1), bb);

    std::shared_ptr<ReturnStatement> ret(new ReturnStatement);

    QVERIFY(ret->getFragment() == nullptr);
    ret->setFragment(frag);
    QVERIFY(ret->getFragment() == frag);
    ret->setFragment(nullptr);
    QVERIFY(ret->getFragment() == nullptr);
}


void StatementTest::testIsNull()
{
    {
        // %eax := -
        std::shared_ptr<ImplicitAssign> imp(new ImplicitAssign(Location::regOf(REG_X86_EAX)));
        QVERIFY(!imp->isNullStatement());
    }

    {
        std::shared_ptr<Assign> asgn(new Assign(Location::regOf(REG_X86_EAX), Location::regOf(REG_X86_ECX)));
        QVERIFY(!asgn->isNullStatement());
    }

    {
        std::shared_ptr<Assign> asgn(new Assign(Location::regOf(REG_X86_EAX), Location::regOf(REG_X86_EAX)));
        QVERIFY(asgn->isNullStatement());
    }

    {
        // 5 %eax := %eax{5}
        std::shared_ptr<Assign> asgn(new Assign(Location::regOf(REG_X86_EAX), Location::regOf(REG_X86_ECX)));
        std::shared_ptr<RefExp> ref = RefExp::get(Location::regOf(REG_X86_EAX), asgn);
        asgn->setRight(ref);

        QVERIFY(asgn->isNullStatement());
    }

    {
        // 5 %eax := %ecx{5}
        std::shared_ptr<Assign> asgn(new Assign(Location::regOf(REG_X86_EAX), Location::regOf(REG_X86_ECX)));
        std::shared_ptr<RefExp> ref = RefExp::get(Location::regOf(REG_X86_ECX), asgn);
        asgn->setRight(ref);

        QVERIFY(asgn->isNullStatement());
    }
}


void StatementTest::testCanPropagateToExp()
{
    QFETCH(SharedExpWrapper, exp);
    QFETCH(bool, canPropagate);

    QCOMPARE(Statement::canPropagateToExp(**exp), canPropagate);
}


void StatementTest::testCanPropagateToExp_data()
{
    const SharedExp eax = Location::regOf(REG_X86_EAX);
    const SharedExp ecx = Location::regOf(REG_X86_ECX);

    const std::shared_ptr<Assign> asgn(new Assign(eax, ecx));
    const std::shared_ptr<Assign> arrayAsgn(new Assign(ArrayType::get(IntegerType::get(32)), eax, ecx));
    const std::shared_ptr<Assign> selfAsgn(new Assign(eax, eax));
    const std::shared_ptr<Assign> selfRef(new Assign(eax, eax));
    const std::shared_ptr<ImplicitAssign> ias(new ImplicitAssign(eax));
    const std::shared_ptr<PhiAssign> phi(new PhiAssign(eax));

    selfRef->setRight(RefExp::get(eax, selfRef));

    QTest::addColumn<SharedExpWrapper>("exp");
    QTest::addColumn<bool>("canPropagate");

    TEST_PROP("%ecx",           ecx,                         false);
    TEST_PROP("%ecx{-}",        RefExp::get(ecx, nullptr),   false);
    TEST_PROP("%eax{ias}",      RefExp::get(eax, ias),       false);
    TEST_PROP("%eax{selfAsgn}", RefExp::get(eax, selfAsgn),  false);
    TEST_PROP("%eax{selfRef}",  RefExp::get(eax, selfRef),   false);
    TEST_PROP("%eax{phi}",      RefExp::get(eax, phi),       false);
    TEST_PROP("%eax{asgn}",     RefExp::get(eax, asgn),      true);
    TEST_PROP("%eax{array}",    RefExp::get(eax, arrayAsgn), false);
}


void StatementTest::testPropagateToThis()
{
    SharedExp eax = Location::regOf(REG_X86_EAX);
    SharedExp ecx = Location::regOf(REG_X86_ECX);
    SharedExp edx = Location::regOf(REG_X86_EDX);

    {
        SharedStmt asgn(new Assign(eax, ecx));
        SharedStmt clone = asgn->clone();

        QVERIFY(!asgn->propagateToThis(3));
        QCOMPARE(asgn->toString(), clone->toString());
    }

    {
        std::shared_ptr<Assign> s10(new Assign(eax, Const::get(0x1000)));
        std::shared_ptr<Assign> s20(new Assign(ecx, Const::get(0)));
        std::shared_ptr<Assign> s30(new Assign(edx, Const::get(0x2000)));

        std::shared_ptr<Assign> asgn(new Assign(
            Location::memOf(RefExp::get(eax, s10)),
            Binary::get(opPlus,
                        RefExp::get(ecx, s20),
                        Location::memOf(RefExp::get(edx, s30)))));

        std::shared_ptr<Assign> expected(new Assign(
            Location::memOf(Const::get(0x1000)),
            Location::memOf(Const::get(0x2000))));

        QVERIFY(asgn->propagateToThis(3));
        QCOMPARE(asgn->toString(), expected->toString());
    }

    {
        std::shared_ptr<Assign> s10(new Assign(eax, Const::get(0x1000)));
        std::shared_ptr<Assign> s20(new Assign(ecx, Const::get(0)));
        std::shared_ptr<Assign> s30(new Assign(edx, Location::memOf(Const::get(2000))));

        std::map<SharedExp, int, lessExpStar> destCounts;

        std::shared_ptr<Assign> asgn(new Assign(
            Location::memOf(RefExp::get(eax, s10)),
            Binary::get(opPlus,
                        RefExp::get(ecx, s20),
                        Location::memOf(RefExp::get(edx, s30)))));

        std::shared_ptr<Assign> expected(new Assign(
            Location::memOf(Const::get(0x1000)),
            Location::memOf(RefExp::get(edx, s30))));

        QVERIFY(asgn->propagateToThis(3));
        QCOMPARE(asgn->toString(), expected->toString());
    }

    {
        std::shared_ptr<Assign> s10(new Assign(eax, Const::get(0x1000)));
        std::shared_ptr<Assign> s20(new Assign(ecx, Const::get(0)));
        std::shared_ptr<Assign> s30(new Assign(eax, Const::get(0x2000)));

        SharedExp ref10 = RefExp::get(eax, s10);
        SharedExp ref20 = RefExp::get(ecx, s20);
        SharedExp ref30 = RefExp::get(edx, s30);

        std::map<SharedExp, int, lessExpStar> destCounts;
        destCounts[ref10] = 2;
        destCounts[ref20] = 1;

        std::shared_ptr<Assign> asgn(new Assign(
            Location::memOf(ref10),
            Binary::get(opPlus,
                        Binary::get(opPlus,
                                    ref20,
                                    Location::memOf(ref10)),
                        ref30)));

        std::shared_ptr<Assign> expected(new Assign(
            Location::memOf(Const::get(0x1000)),
            Binary::get(opPlus,
                        Location::memOf(Const::get(0x1000)),
                        Const::get(0x2000))));

        QVERIFY(asgn->propagateToThis(2, &destCounts));
        QCOMPARE(asgn->toString(), expected->toString());
    }

    // TODO: Check if multi-propagation respects the -l switch
}


void StatementTest::testPropagateFlagsToThis()
{
    SharedExp eax   = Location::regOf(REG_X86_EAX);
    SharedExp ecx   = Location::regOf(REG_X86_ECX);
    SharedExp edx   = Location::regOf(REG_X86_ECX);
    SharedExp flags = Terminal::get(opFlags);
    SharedExp st0   = Location::regOf(REG_X86_ST0);
    SharedExp st1   = Location::regOf(REG_X86_ST1);

    {
        SharedStmt asgn(new Assign(eax, ecx));
        asgn->setNumber(1);
        SharedStmt clone = asgn->clone();

        QVERIFY(!asgn->propagateFlagsToThis());
        QCOMPARE(asgn->toString(), clone->toString());
    }

    {
        SharedStmt def(new Assign(flags, Const::get(0)));
        SharedStmt asgn(new Assign(eax, RefExp::get(flags, def)));
        SharedStmt expected(new Assign(eax, Const::get(0)));

        def->setNumber(1);
        asgn->setNumber(2);
        expected->setNumber(2);

        QVERIFY(asgn->propagateFlagsToThis());
        QCOMPARE(asgn->toString(), expected->toString());
    }

    {
        SharedStmt def(new PhiAssign(flags));
        SharedStmt asgn(new Assign(eax, RefExp::get(flags, def)));
        SharedStmt expected = asgn->clone();

        def->setNumber(1);
        asgn->setNumber(2);
        expected->setNumber(2);

        QVERIFY(!asgn->propagateFlagsToThis());
        QCOMPARE(asgn->toString(), expected->toString());
    }

    {
        SharedStmt def(new Assign(flags, ecx));
        SharedStmt asgn(new Assign(eax, RefExp::get(flags, def)));
        SharedStmt expected = asgn->clone();

        def->setNumber(1);
        asgn->setNumber(2);
        expected->setNumber(2);

        QVERIFY(!asgn->propagateFlagsToThis());
        QCOMPARE(asgn->toString(), expected->toString());
    }

    {
        SharedStmt def(new Assign(flags, makeFlagCall("SUBFLAGSFL", st0, st1)));
        SharedStmt asgn(new Assign(eax, RefExp::get(Terminal::get(opCF), def)));
        SharedStmt expected(new Assign(eax, Binary::get(opLess, st0, st1)));

        def->setNumber(1);
        asgn->setNumber(2);
        expected->setNumber(2);

        QVERIFY(asgn->propagateFlagsToThis());
        QCOMPARE(asgn->toString(), expected->toString());
    }

    {
        SharedStmt def(new Assign(flags, makeFlagCall("SUBFLAGSFL", st0, st1)));
        SharedStmt asgn(new Assign(eax, RefExp::get(Terminal::get(opZF), def)));
        SharedStmt expected(new Assign(eax, Binary::get(opEquals, st0, st1)));

        def->setNumber(1);
        asgn->setNumber(2);
        expected->setNumber(2);

        QVERIFY(asgn->propagateFlagsToThis());
        QCOMPARE(asgn->toString(), expected->toString());
    }

    {
        SharedStmt def(new Assign(flags, makeFlagCall("SUBFLAGSFL", st0, st1)));
        SharedStmt asgn(new Assign(eax, RefExp::get(Terminal::get(opOF), def)));
        SharedStmt expected(new Assign(eax, makeFlagCall("SUBFLAGSFL", st0, st1)));

        def->setNumber(1);
        asgn->setNumber(2);
        expected->setNumber(2);

        QVERIFY(asgn->propagateFlagsToThis());
        QCOMPARE(asgn->toString(), expected->toString());
    }

    {
        SharedStmt def(new Assign(flags, makeFlagCall("SUBFLAGS", eax, ecx, edx)));
        SharedStmt asgn(new Assign(eax, RefExp::get(Terminal::get(opCF), def)));
        SharedStmt expected(new Assign(eax, Binary::get(opLessUns, eax, ecx)));

        def->setNumber(1);
        asgn->setNumber(2);
        expected->setNumber(2);

        QVERIFY(asgn->propagateFlagsToThis());
        QCOMPARE(asgn->toString(), expected->toString());
    }

    {
        SharedStmt def(new Assign(flags, makeFlagCall("SUBFLAGS", eax, ecx, edx)));
        SharedStmt asgn(new Assign(eax, RefExp::get(Terminal::get(opZF), def)));
        SharedStmt expected(new Assign(eax, Binary::get(opEquals, edx, Const::get(0))));

        def->setNumber(1);
        asgn->setNumber(2);
        expected->setNumber(2);

        QVERIFY(asgn->propagateFlagsToThis());
        QCOMPARE(asgn->toString(), expected->toString());
    }

    {
        SharedStmt def(new Assign(flags, makeFlagCall("SUBFLAGS", eax, ecx, edx)));
        SharedStmt asgn(new Assign(eax, RefExp::get(Terminal::get(opNF), def)));
        SharedStmt expected(new Assign(eax, Binary::get(opLess, edx, Const::get(0))));

        def->setNumber(1);
        asgn->setNumber(2);
        expected->setNumber(2);

        QVERIFY(asgn->propagateFlagsToThis());
        QCOMPARE(asgn->toString(), expected->toString());
    }

    {
        SharedStmt def(new Assign(flags, makeFlagCall("SUBFLAGS", eax, ecx, edx)));
        SharedStmt asgn(new Assign(eax, RefExp::get(Terminal::get(opOF), def)));
        SharedStmt expected(new Assign(eax,
            Binary::get(opOr,
                        Binary::get(opAnd,
                                    Binary::get(opAnd, Binary::get(opLess, eax, Const::get(0)),
                                                Binary::get(opGtrEq, ecx, Const::get(0))),
                                    Binary::get(opGtrEq, edx, Const::get(0))),
                        Binary::get(opAnd,
                                    Binary::get(opAnd, Binary::get(opGtrEq, eax, Const::get(0)),
                                                Binary::get(opLess, ecx, Const::get(0))),
                                    Binary::get(opLess, edx, Const::get(0))))));

        def->setNumber(1);
        asgn->setNumber(2);
        expected->setNumber(2);

        QVERIFY(asgn->propagateFlagsToThis());
        QCOMPARE(asgn->toString(), expected->toString());
    }

    {
        SharedStmt def(new Assign(flags, makeFlagCall("SUBFLAGS", eax, ecx, edx)));
        SharedStmt asgn(new Assign(eax, RefExp::get(Terminal::get(opDF), def)));
        SharedStmt expected(new Assign(eax, RefExp::get(Terminal::get(opDF), def)));

        def->setNumber(1);
        asgn->setNumber(2);
        expected->setNumber(2);

        QVERIFY(!asgn->propagateFlagsToThis());
        QCOMPARE(asgn->toString(), expected->toString());
    }

    {
        SharedStmt def(new Assign(flags, makeFlagCall("LOGICALFLAGS", eax)));
        SharedStmt asgn(new Assign(eax, RefExp::get(Terminal::get(opNF), def)));
        SharedStmt expected(new Assign(eax, Binary::get(opLess, eax, Const::get(0))));

        def->setNumber(1);
        asgn->setNumber(2);
        expected->setNumber(2);

        QVERIFY(asgn->propagateFlagsToThis());
        QCOMPARE(asgn->toString(), expected->toString());
    }

    {
        SharedStmt def(new Assign(flags, makeFlagCall("LOGICALFLAGS", eax)));
        SharedStmt asgn(new Assign(eax, RefExp::get(Terminal::get(opZF), def)));
        SharedStmt expected(new Assign(eax, Binary::get(opEquals, eax, Const::get(0))));

        def->setNumber(1);
        asgn->setNumber(2);
        expected->setNumber(2);

        QVERIFY(asgn->propagateFlagsToThis());
        QCOMPARE(asgn->toString(), expected->toString());
    }

    {
        SharedStmt def(new Assign(flags, makeFlagCall("LOGICALFLAGS", eax)));
        SharedStmt asgn(new Assign(eax, RefExp::get(Terminal::get(opCF), def)));
        SharedStmt expected(new Assign(eax, Const::get(0)));

        def->setNumber(1);
        asgn->setNumber(2);
        expected->setNumber(2);

        QVERIFY(asgn->propagateFlagsToThis());
        QCOMPARE(asgn->toString(), expected->toString());
    }

    {
        SharedStmt def(new Assign(flags, makeFlagCall("LOGICALFLAGS", eax)));
        SharedStmt asgn(new Assign(eax, RefExp::get(Terminal::get(opOF), def)));
        SharedStmt expected(new Assign(eax, Const::get(0)));

        def->setNumber(1);
        asgn->setNumber(2);
        expected->setNumber(2);

        QVERIFY(asgn->propagateFlagsToThis());
        QCOMPARE(asgn->toString(), expected->toString());
    }

    {
        SharedStmt def(new Assign(flags, makeFlagCall("LOGICALFLAGS", eax)));
        SharedStmt asgn(new Assign(eax, RefExp::get(Terminal::get(opDF), def)));
        SharedStmt expected(new Assign(eax, RefExp::get(Terminal::get(opDF), def)));

        def->setNumber(1);
        asgn->setNumber(2);
        expected->setNumber(2);

        QVERIFY(!asgn->propagateFlagsToThis());
        QCOMPARE(asgn->toString(), expected->toString());
    }

    {
        SharedStmt def(new Assign(flags, makeFlagCall("INCDECFLAGS", eax)));
        SharedStmt asgn(new Assign(eax, RefExp::get(Terminal::get(opOF), def)));
        SharedStmt expected(new Assign(eax, Const::get(0)));

        def->setNumber(1);
        asgn->setNumber(2);
        expected->setNumber(2);

        QVERIFY(asgn->propagateFlagsToThis());
        QCOMPARE(asgn->toString(), expected->toString());
    }

    {
        SharedStmt def(new Assign(flags, makeFlagCall("INCDECFLAGS", eax)));
        SharedStmt asgn(new Assign(eax, RefExp::get(Terminal::get(opOF), def)));
        SharedStmt expected(new Assign(eax, Const::get(0)));

        def->setNumber(1);
        asgn->setNumber(2);
        expected->setNumber(2);

        QVERIFY(asgn->propagateFlagsToThis());
        QCOMPARE(asgn->toString(), expected->toString());
    }

    {
        SharedStmt def(new Assign(flags, makeFlagCall("INCDECFLAGS", eax)));
        SharedStmt asgn(new Assign(eax, RefExp::get(Terminal::get(opZF), def)));
        SharedStmt expected(new Assign(eax, Binary::get(opEquals, eax, Const::get(0))));

        def->setNumber(1);
        asgn->setNumber(2);
        expected->setNumber(2);

        QVERIFY(asgn->propagateFlagsToThis());
        QCOMPARE(asgn->toString(), expected->toString());
    }

    {
        SharedStmt def(new Assign(flags, makeFlagCall("INCDECFLAGS", eax)));
        SharedStmt asgn(new Assign(eax, RefExp::get(Terminal::get(opNF), def)));
        SharedStmt expected(new Assign(eax,Binary::get(opLess, eax, Const::get(0))));

        def->setNumber(1);
        asgn->setNumber(2);
        expected->setNumber(2);

        QVERIFY(asgn->propagateFlagsToThis());
        QCOMPARE(asgn->toString(), expected->toString());
    }

    {
        SharedStmt def(new Assign(flags, makeFlagCall("INCDECFLAGS", eax)));
        SharedStmt asgn(new Assign(eax, RefExp::get(Terminal::get(opDF), def)));
        SharedStmt expected(new Assign(eax, RefExp::get(Terminal::get(opDF), def)));

        def->setNumber(1);
        asgn->setNumber(2);
        expected->setNumber(2);

        QVERIFY(!asgn->propagateFlagsToThis());
        QCOMPARE(asgn->toString(), expected->toString());
    }

    // TODO: Test replaceRef
}


void StatementTest::testIsAssign()
{
    std::shared_ptr<Assign> a(new Assign(Location::regOf(REG_X86_DX), Const::get(99)));
    std::shared_ptr<CallStatement> call(new CallStatement(Address(0x1000)));

    QVERIFY(a->isAssign());
    QVERIFY(!call->isAssign());
}


void StatementTest::testIsFlagAssgn()
{
    // FLAG addFlags(r2 , 99)
    std::shared_ptr<Assign> fc(new Assign(Terminal::get(opFlags),
                                          Binary::get(opFlagCall,
                                                      Const::get("addFlags"),
                                                      Binary::get(opList,
                                                                  Location::regOf(REG_X86_DX),
                                                                  Const::get(99)))));
    std::shared_ptr<CallStatement> call(new CallStatement(Address(0x1000)));
    std::shared_ptr<BranchStatement> br(new BranchStatement(Address(0x1000)));
    std::shared_ptr<Assign> as(new Assign(Location::regOf(REG_X86_CL),
                                          Binary::get(opPlus,
                                                      Location::regOf(REG_X86_DL),
                                                      Const::get(4))));

    QVERIFY(fc->isFlagAssign());
    QVERIFY(!call->isFlagAssign());
    QVERIFY(!br->isFlagAssign());
    QVERIFY(!as->isFlagAssign());
}


void StatementTest::testAddUsedLocsAssign()
{
    {
        // m[r28-4] := m[r28-8] * r26
        std::shared_ptr<Assign> a(new Assign(
            Location::memOf(Binary::get(opMinus,
                                        Location::regOf(REG_X86_ESP),
                                        Const::get(4))),
            Binary::get(opMult,
                        Location::memOf(Binary::get(opMinus,
                                                    Location::regOf(REG_X86_ESP),
                                                    Const::get(8))),
                        Location::regOf(REG_X86_EDX))));
        a->setNumber(1);

        LocationSet l;
        a->addUsedLocs(l);

        QCOMPARE(l.toString(), "r26, r28, m[r28 - 8]");
    }

    {

        std::shared_ptr<GotoStatement> g(new GotoStatement(Location::memOf(Location::regOf(REG_X86_EDX))));
        g->setNumber(55);

        LocationSet l;
        g->addUsedLocs(l);

        QCOMPARE(l.toString(), "r26, m[r26]");
    }
}


void StatementTest::testAddUsedLocsBranch()
{
    // BranchStatement with dest m[r26{99}]{55}, condition %flags
    std::shared_ptr<GotoStatement> g(new GotoStatement(Address(0x1000)));
    g->setNumber(55);

    std::shared_ptr<BranchStatement> b(new BranchStatement(Address(0x1000)));
    b->setNumber(99);
    b->setDest(RefExp::get(Location::memOf(RefExp::get(Location::regOf(REG_X86_EDX), b)), g));
    b->setCondExpr(Terminal::get(opFlags));

    LocationSet l;
    b->addUsedLocs(l);

    QCOMPARE(l.toString(), "r26{99}, m[r26{99}]{55}, %flags");
}


void StatementTest::testAddUsedLocsCase()
{
    // CaseStatement with dest = m[r26], switchVar = m[r28 - 12]
    std::shared_ptr<CaseStatement> c(new CaseStatement(Location::memOf(Location::regOf(REG_X86_EDX))));

    std::unique_ptr<SwitchInfo> si(new SwitchInfo);
    si->switchExp = Location::memOf(Binary::get(opMinus, Location::regOf(REG_X86_ESP), Const::get(12)));
    c->setSwitchInfo(std::move(si));

    LocationSet l;
    c->addUsedLocs(l);

    QCOMPARE(l.toString(), "r26, r28, m[r28 - 12], m[r26]");
}


void StatementTest::testAddUsedLocsCall()
{
    // CallStatement with dest = m[r26], params = m[r27], r28{55}, defines r31, m[r24]
    std::shared_ptr<GotoStatement> g(new GotoStatement(Address(0x1000)));
    g->setNumber(55);

    std::shared_ptr<CallStatement> ca(new CallStatement(Location::memOf(Location::regOf(REG_X86_EDX))));
    StatementList argl;

    argl.append(std::make_shared<Assign>(Location::regOf(REG_X86_AL), Location::memOf(Location::regOf(REG_X86_EBX))));
    argl.append(std::make_shared<Assign>(Location::regOf(REG_X86_CL), RefExp::get(Location::regOf(REG_X86_ESP), g)));
    ca->setArguments(argl);

    ca->addDefine(std::make_shared<ImplicitAssign>(Location::regOf(REG_X86_EDI)));
    ca->addDefine(std::make_shared<ImplicitAssign>(Location::regOf(REG_X86_EAX)));

    LocationSet l;
    ca->addUsedLocs(l);

    QCOMPARE(l.toString(), "r26, r27, m[r26], m[r27], r28{55}");
}


void StatementTest::testAddUsedLocsReturn()
{
    // ReturnStatement with returns r31, m[r24], m[r25]{55} + r[26]{99}]
    std::shared_ptr<GotoStatement> g(new GotoStatement(Address(0x0800)));
    g->setNumber(55);

    std::shared_ptr<BranchStatement> b(new BranchStatement(Address(0x0800)));
    b->setNumber(99);

    std::shared_ptr<ReturnStatement> r(new ReturnStatement);
    r->addReturn(std::make_shared<Assign>(Location::regOf(REG_X86_EDI), Const::get(100)));
    r->addReturn(std::make_shared<Assign>(Location::memOf(Location::regOf(REG_X86_EAX)), Const::get(0)));
    r->addReturn(std::make_shared<Assign>(
                     Location::memOf(Binary::get(opPlus, RefExp::get(Location::regOf(REG_X86_ECX), g), RefExp::get(Location::regOf(REG_X86_EDX), b))),
                     Const::get(5)));

    LocationSet   l;
    r->addUsedLocs(l);

    QCOMPARE(l.toString(), "r24, r25{55}, r26{99}");
}


void StatementTest::testAddUsedLocsBool()
{
    {
        // Boolstatement with condition m[r24] = r25, dest m[r26]
        SharedExp lhs = Location::memOf(Location::regOf(REG_X86_EDX));
        SharedExp cond = Binary::get(opEquals, Location::memOf(Location::regOf(REG_X86_EAX)), Location::regOf(REG_X86_ECX));

        std::shared_ptr<BoolAssign> bs(new BoolAssign(lhs, BranchType::JE, cond));

        LocationSet l;
        bs->addUsedLocs(l);

        QCOMPARE(l.toString(), "r24, r25, r26, m[r24]");
    }

    {
        // m[local21 + 16] := phi{0, 372}
        SharedExp base = Location::memOf(Binary::get(opPlus, Location::local("local21", nullptr), Const::get(16)));
        std::shared_ptr<Assign> s372(new Assign(base, Const::get(0)));
        s372->setNumber(372);

        std::shared_ptr<PhiAssign> pa(new PhiAssign(base));
        pa->putAt(nullptr, nullptr, base); // 0
        pa->putAt(nullptr, s372, base);    // 1

        LocationSet l;
        pa->addUsedLocs(l);

        // Note: phis were not considered to use blah if they ref m[blah], so local21 was not considered used
        QCOMPARE(l.toString(), "local21, m[local21 + 16]{372}");
    }

    // m[r28{-} - 4] := -
    {
        auto ia = std::make_shared<ImplicitAssign>(Location::memOf(Binary::get(opMinus,
                                               RefExp::get(Location::regOf(REG_X86_ESP), nullptr),
                                               Const::get(4))));

        LocationSet l;
        ia->addUsedLocs(l);

        QCOMPARE(l.toString(), "r28{-}");
    }
}


void StatementTest::testBypass()
{
    QVERIFY(m_project.loadBinaryFile(GLOBAL1_X86));

    Prog *prog = m_project.getProg();
    IFrontEnd *fe = prog->getFrontEnd();
    assert(fe != nullptr);

    Type::clearNamedTypes();
    prog->setFrontEnd(fe);

    QVERIFY(fe->disassembleEntryPoints());
    QVERIFY(fe->disassembleAll());

    bool    gotMain;
    Address addr = fe->findMainEntryPoint(gotMain);
    QVERIFY(addr != Address::INVALID);

    UserProc *proc = static_cast<UserProc *>(prog->getFunctionByName("foo2"));
    QVERIFY(proc != nullptr);

    proc->promoteSignature(); // Make sure it's an X86Signature (needed for bypassing)

    PassManager::get()->executePass(PassID::StatementInit, proc);
    PassManager::get()->executePass(PassID::Dominators, proc);

    // Note: we need to have up to date call defines before transforming to SSA form,
    // because otherwise definitions of calls get ignored.
    PassManager::get()->executePass(PassID::CallDefineUpdate, proc);
    PassManager::get()->executePass(PassID::BlockVarRename, proc);

    proc->numberStatements();

    // Find various needed statements
    StatementList stmts;
    proc->getStatements(stmts);
    StatementList::iterator it = stmts.begin();

    while (it != stmts.end() && !(*it)->isCall()) {
        ++it;
    }
    QVERIFY(it != stmts.end());
    SharedStmt s19 = *std::next(it, 2);
    QVERIFY(s19->getKind() == StmtType::Assign);

    QCOMPARE(s19->toString(), "  19 *32* r28 := r28{17} + 16");

    s19->bypass();        // r28 should bypass the call
    QCOMPARE(s19->toString(), "  19 *32* r28 := r28{15} + 20");

    // should do nothing because r28{15} is the only reference to r28 that reaches the call
    s19->bypass();
    QCOMPARE(s19->toString(), "  19 *32* r28 := r28{15} + 20");
}


QTEST_GUILESS_MAIN(StatementTest)

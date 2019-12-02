#pragma region License
/*
 * This file is part of the Boomerang Decompiler.
 *
 * See the file "LICENSE.TERMS" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 */
#pragma endregion License
#include "SPARCFrontEndTest.h"

#include "boomerang-plugins/frontend/sparc/SPARCFrontEnd.h"

#include "boomerang/db/BasicBlock.h"
#include "boomerang/db/Prog.h"
#include "boomerang/db/proc/ProcCFG.h"
#include "boomerang/db/proc/UserProc.h"
#include "boomerang/passes/PassManager.h"
#include "boomerang/ssl/RTL.h"
#include "boomerang/util/Types.h"
#include "boomerang/util/log/Log.h"

#include <QDebug>


#define HELLO_SPARC     getFullSamplePath("sparc/hello")
#define BRANCH_SPARC    getFullSamplePath("sparc/branch")


void SPARCFrontendTest::test1()
{
    QVERIFY(m_project.loadBinaryFile(HELLO_SPARC));

    Prog      *prog = m_project.getProg();
    SPARCFrontEnd *fe = dynamic_cast<SPARCFrontEnd *>(prog->getFrontEnd());
    QVERIFY(fe != nullptr);

    bool    gotMain;
    Address addr = fe->findMainEntryPoint(gotMain);
    QVERIFY(addr != Address::INVALID);

    // Decode first instruction
    {
        MachineInstruction insn;
        LiftedInstruction lifted;

        QVERIFY(fe->decodeInstruction(addr, insn, lifted));
        QVERIFY(lifted.getFirstRTL() != nullptr);

        const QString expected = "0x00010684    0 *32* tmp := r14 - 112\n"
                                 "              0 *32* m[r14] := r16\n"
                                 "              0 *32* m[r14 + 4] := r17\n"
                                 "              0 *32* m[r14 + 8] := r18\n"
                                 "              0 *32* m[r14 + 12] := r19\n"
                                 "              0 *32* m[r14 + 16] := r20\n"
                                 "              0 *32* m[r14 + 20] := r21\n"
                                 "              0 *32* m[r14 + 24] := r22\n"
                                 "              0 *32* m[r14 + 28] := r23\n"
                                 "              0 *32* m[r14 + 32] := r24\n"
                                 "              0 *32* m[r14 + 36] := r25\n"
                                 "              0 *32* m[r14 + 40] := r26\n"
                                 "              0 *32* m[r14 + 44] := r27\n"
                                 "              0 *32* m[r14 + 48] := r28\n"
                                 "              0 *32* m[r14 + 52] := r29\n"
                                 "              0 *32* m[r14 + 56] := r30\n"
                                 "              0 *32* m[r14 + 60] := r31\n"
                                 "              0 *32* r24 := r8\n"
                                 "              0 *32* r25 := r9\n"
                                 "              0 *32* r26 := r10\n"
                                 "              0 *32* r27 := r11\n"
                                 "              0 *32* r28 := r12\n"
                                 "              0 *32* r29 := r13\n"
                                 "              0 *32* r30 := r14\n"
                                 "              0 *32* r31 := r15\n"
                                 "              0 *32* r14 := tmp\n";
        QCOMPARE(lifted.getFirstRTL()->toString(), expected);
        addr += insn.m_size;
    }


    {
        MachineInstruction insn;
        LiftedInstruction lifted;

        QVERIFY(fe->decodeInstruction(addr, insn, lifted));
        const QString expected = "0x00010688    0 *32* r8 := 0x10400\n";
        QCOMPARE(lifted.getFirstRTL()->toString(), expected);
        addr += insn.m_size;
    }

    {
        MachineInstruction insn;
        LiftedInstruction lifted;

        QVERIFY(fe->decodeInstruction(addr, insn, lifted));
        const QString expected = "0x0001068c    0 *32* r8 := r8 | 848\n";
        QCOMPARE(lifted.getFirstRTL()->toString(), expected);
    }
}


void SPARCFrontendTest::test2()
{
    QVERIFY(m_project.loadBinaryFile(HELLO_SPARC));
    Prog *prog = m_project.getProg();
    SPARCFrontEnd *fe = dynamic_cast<SPARCFrontEnd *>(prog->getFrontEnd());
    QVERIFY(fe != nullptr);

    {
        MachineInstruction insn;
        LiftedInstruction lifted;

        QVERIFY(fe->decodeInstruction(Address(0x00010690), insn, lifted));

        // This call is to out of range of the program's text limits
        // (to the Program Linkage Table (PLT), calling printf). This is quite normal.
        const QString expected = "0x00010690    0 CALL printf(\n"
                                 "              )\n"
                                 "              Reaching definitions: <None>\n"
                                 "              Live variables: <None>\n";
        QCOMPARE(lifted.getFirstRTL()->toString(), expected);
    }

    {
        MachineInstruction insn;
        LiftedInstruction lifted;

        QVERIFY(fe->decodeInstruction(Address(0x00010694), insn, lifted));
        const QString expected = QString("0x00010694\n");
        QCOMPARE(lifted.getFirstRTL()->toString(), expected);
    }

    {
        MachineInstruction insn;
        LiftedInstruction lifted;

        QVERIFY(fe->decodeInstruction(Address(0x00010698), insn, lifted));
        const QString expected = "0x00010698    0 *32* r8 := 0\n";
        QCOMPARE(lifted.getFirstRTL()->toString(), expected);
    }

    {
        MachineInstruction insn;
        LiftedInstruction lifted;

        QVERIFY(fe->decodeInstruction(Address(0x0001069C), insn, lifted));
        const QString expected = "0x0001069c    0 *32* r24 := r8\n";
        QCOMPARE(lifted.getFirstRTL()->toString(), expected);
    }
}


void SPARCFrontendTest::test3()
{
    QVERIFY(m_project.loadBinaryFile(HELLO_SPARC));
    Prog *prog = m_project.getProg();
    SPARCFrontEnd *fe = dynamic_cast<SPARCFrontEnd *>(prog->getFrontEnd());
    QVERIFY(fe != nullptr);

    {
        MachineInstruction insn;
        LiftedInstruction lifted;

        QVERIFY(fe->decodeInstruction(Address(0x000106a0), insn, lifted));
        const QString expected = "0x000106a0\n";
        QCOMPARE(lifted.getFirstRTL()->toString(), expected);
    }

    {
        MachineInstruction insn;
        LiftedInstruction lifted;

        QVERIFY(fe->decodeInstruction(Address(0x000106a4), insn, lifted));
        const QString expected = "0x000106a4    0 RET\n"
                                 "              Modifieds: <None>\n"
                                 "              Reaching definitions: <None>\n";
        QCOMPARE(lifted.getFirstRTL()->toString(), expected);
    }

    {
        MachineInstruction insn;
        LiftedInstruction lifted;

        QVERIFY(fe->decodeInstruction(Address(0x000106a8), insn, lifted));
        const QString expected = "0x000106a8    0 *32* tmp := 0\n"
                                 "              0 *32* r8 := r24\n"
                                 "              0 *32* r9 := r25\n"
                                 "              0 *32* r10 := r26\n"
                                 "              0 *32* r11 := r27\n"
                                 "              0 *32* r12 := r28\n"
                                 "              0 *32* r13 := r29\n"
                                 "              0 *32* r14 := r30\n"
                                 "              0 *32* r15 := r31\n"
                                 "              0 *32* r0 := tmp\n"
                                 "              0 *32* r16 := m[r14]\n"
                                 "              0 *32* r17 := m[r14 + 4]\n"
                                 "              0 *32* r18 := m[r14 + 8]\n"
                                 "              0 *32* r19 := m[r14 + 12]\n"
                                 "              0 *32* r20 := m[r14 + 16]\n"
                                 "              0 *32* r21 := m[r14 + 20]\n"
                                 "              0 *32* r22 := m[r14 + 24]\n"
                                 "              0 *32* r23 := m[r14 + 28]\n"
                                 "              0 *32* r24 := m[r14 + 32]\n"
                                 "              0 *32* r25 := m[r14 + 36]\n"
                                 "              0 *32* r26 := m[r14 + 40]\n"
                                 "              0 *32* r27 := m[r14 + 44]\n"
                                 "              0 *32* r28 := m[r14 + 48]\n"
                                 "              0 *32* r29 := m[r14 + 52]\n"
                                 "              0 *32* r30 := m[r14 + 56]\n"
                                 "              0 *32* r31 := m[r14 + 60]\n"
                                 "              0 *32* r0 := tmp\n";
        compareLongStrings(lifted.getFirstRTL()->toString(), expected);
    }
}


void SPARCFrontendTest::testBranch()
{
    QVERIFY(m_project.loadBinaryFile(BRANCH_SPARC));
    Prog *prog = m_project.getProg();
    SPARCFrontEnd *fe = dynamic_cast<SPARCFrontEnd *>(prog->getFrontEnd());
    QVERIFY(fe != nullptr);

    // bne
    {
        MachineInstruction insn;
        LiftedInstruction lifted;

        QVERIFY(fe->decodeInstruction(Address(0x00010ab0), insn, lifted));
        const QString expected = "0x00010ab0    0 BRANCH 0x00010ac8, condition not equals\n"
                                 "High level: %flags\n";
        QCOMPARE(lifted.getFirstRTL()->toString(), expected);
    }

    // bg
    {
        MachineInstruction insn;
        LiftedInstruction lifted;

        QVERIFY(fe->decodeInstruction(Address(0x00010af8), insn, lifted));
        const QString expected = "0x00010af8    0 BRANCH 0x00010b10, condition signed greater\n"
                                 "High level: %flags\n";
        QCOMPARE(lifted.getFirstRTL()->toString(), expected);
    }

    // bleu
    {
        MachineInstruction insn;
        LiftedInstruction lifted;

        QVERIFY(fe->decodeInstruction(Address(0x00010b44), insn, lifted));
        const QString expected = "0x00010b44    0 BRANCH 0x00010b54, condition unsigned less or equals\n"
                                 "High level: %flags\n";
        QCOMPARE(lifted.getFirstRTL()->toString(), expected);
    }
}


void SPARCFrontendTest::testDelaySlot()
{
    QVERIFY(m_project.loadBinaryFile(BRANCH_SPARC));
    Prog *prog = m_project.getProg();
    SPARCFrontEnd *fe = dynamic_cast<SPARCFrontEnd *>(prog->getFrontEnd());
    QVERIFY(fe != nullptr);

    // disassembly calls readLibraryCatalog(), which needs to have definitions
    // for non-SPARC architectures cleared
    Type::clearNamedTypes();
    QVERIFY(fe->disassembleEntryPoints());

    bool    gotMain;
    const Address mainAddr = fe->findMainEntryPoint(gotMain);
    QVERIFY(mainAddr != Address::INVALID);
    QVERIFY(gotMain);

    Module      *m = prog->getOrInsertModule("test");

    UserProc    proc(mainAddr, "testDelaySlot", m);
    QVERIFY(fe->disassembleFragment(&proc, mainAddr));

    PassManager::get()->executePass(PassID::StatementInit, &proc);

    ProcCFG        *cfg = proc.getCFG();
    ProcCFG::iterator it = cfg->begin();

    {
        QVERIFY(it != cfg->end());
        const IRFragment *frag = *it;
        QVERIFY(frag != nullptr);
        const QString expected = "Call BB:\n"
                                 "  in edges: \n"
                                 "  out edges: 0x00010a98 \n"
                                 "0x00010a80    0 *32* tmp := r14 - 120\n"
                                 "              0 *32* m[r14] := r16\n"
                                 "              0 *32* m[r14 + 4] := r17\n"
                                 "              0 *32* m[r14 + 8] := r18\n"
                                 "              0 *32* m[r14 + 12] := r19\n"
                                 "              0 *32* m[r14 + 16] := r20\n"
                                 "              0 *32* m[r14 + 20] := r21\n"
                                 "              0 *32* m[r14 + 24] := r22\n"
                                 "              0 *32* m[r14 + 28] := r23\n"
                                 "              0 *32* m[r14 + 32] := r24\n"
                                 "              0 *32* m[r14 + 36] := r25\n"
                                 "              0 *32* m[r14 + 40] := r26\n"
                                 "              0 *32* m[r14 + 44] := r27\n"
                                 "              0 *32* m[r14 + 48] := r28\n"
                                 "              0 *32* m[r14 + 52] := r29\n"
                                 "              0 *32* m[r14 + 56] := r30\n"
                                 "              0 *32* m[r14 + 60] := r31\n"
                                 "              0 *32* r24 := r8\n"
                                 "              0 *32* r25 := r9\n"
                                 "              0 *32* r26 := r10\n"
                                 "              0 *32* r27 := r11\n"
                                 "              0 *32* r28 := r12\n"
                                 "              0 *32* r29 := r13\n"
                                 "              0 *32* r30 := r14\n"
                                 "              0 *32* r31 := r15\n"
                                 "              0 *32* r14 := tmp\n"
                                 "0x00010a84    0 *32* r16 := 0x11400\n"
                                 "0x00010a88    0 *32* r16 := r16 | 808\n"
                                 "0x00010a8c    0 *32* r8 := r16\n"
                                 "0x00010a90    0 *32* tmp := r30\n"
                                 "              0 *32* r9 := r30 - 20\n"
                                 "0x00010a90    0 CALL scanf(\n"
                                 "              )\n"
                                 "              Reaching definitions: <None>\n"
                                 "              Live variables: <None>\n";

        compareLongStrings(frag->toString(), expected);
    }

    {
        QVERIFY(it != cfg->end());
        const IRFragment *frag = *(++it);
        QVERIFY(frag != nullptr);
        const QString expected = "Call BB:\n"
                                 "  in edges: 0x00010a90(0x00010a80) \n"
                                 "  out edges: 0x00010aa4 \n"
                                 "0x00010a98    0 *32* r8 := r16\n"
                                 "0x00010a9c    0 *32* tmp := r30\n"
                                 "              0 *32* r9 := r30 - 24\n"
                                 "0x00010a9c    0 CALL scanf(\n"
                                 "              )\n"
                                 "              Reaching definitions: <None>\n"
                                 "              Live variables: <None>\n";

        compareLongStrings(frag->toString(), expected);
    }

    {
        QVERIFY(it != cfg->end());
        const IRFragment *frag = *(++it);
        QVERIFY(frag != nullptr);
        const QString expected = "Twoway BB:\n"
                                 "  in edges: 0x00010a9c(0x00010a98) \n"
                                 "  out edges: 0x00010ac8 0x00010ab8 \n"
                                 "0x00010aa4    0 *32* r8 := m[r30 - 20]\n"
                                 "0x00010aa8    0 *32* r16 := 5\n"
                                 "0x00010aac    0 *v* %flags := SUBFLAGS( r16, r8, r16 - r8 )\n"
                                 "0x00010ab0    0 *32* r8 := 0x11400\n"
                                 "0x00010ab0    0 BRANCH 0x00010ac8, condition not equals\n"
                                 "High level: %flags\n";
        compareLongStrings(frag->toString(), expected);
    }

    {
        QVERIFY(it != cfg->end());
        const IRFragment *frag = *(++it);
        QVERIFY(frag != nullptr);
        const QString expected = "Call BB:\n"
                                 "  in edges: 0x00010ab0(0x00010aa4) \n"
                                 "  out edges: 0x00010ac0 \n"
                                 "0x00010ab8    0 *32* r8 := r8 | 816\n"
                                 "0x00010ab8    0 CALL printf(\n"
                                 "              )\n"
                                 "              Reaching definitions: <None>\n"
                                 "              Live variables: <None>\n";

        compareLongStrings(frag->toString(), expected);
    }

    {
        QVERIFY(it != cfg->end());
        const IRFragment *frag = *(++it);
        QVERIFY(frag != nullptr);

        const QString expected = "Fall BB:\n"
                                 "  in edges: 0x00010ab8(0x00010ab8) \n"
                                 "  out edges: 0x00010ac8 \n"
                                 "0x00010ac0    0 *32* r8 := m[r30 - 20]\n"
                                 "0x00010ac4    0 *v* %flags := SUBFLAGS( r16, r8, r16 - r8 )\n";
        compareLongStrings(frag->toString(), expected);
    }

    {
        QVERIFY(it != cfg->end());
        const IRFragment *frag = *(++it);
        QVERIFY(frag != nullptr);
        const QString expected = "Twoway BB:\n"
                                 "  in edges: 0x00010ab0(0x00010aa4) 0x00010ac4(0x00010ac0) \n"
                                 "  out edges: 0x00010ad8 0x00010ad0 \n"
                                 "0x00010ac8    0 *32* r8 := 0x11400\n"
                                 "0x00010ac8    0 BRANCH 0x00010ad8, condition equals\n"
                                 "High level: %flags\n";
        compareLongStrings(frag->toString(), expected);
    }
}


QTEST_GUILESS_MAIN(SPARCFrontendTest)

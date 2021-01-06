#pragma region License
/*
 * This file is part of the Boomerang Decompiler.
 *
 * See the file "LICENSE.TERMS" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 */
#pragma endregion License
#include "CapstonePPCDecoderTest.h"

#include "boomerang/ssl/RTL.h"
#include "boomerang/util/Types.h"


struct InstructionData
{
public:
    Byte data[5];
};

Q_DECLARE_METATYPE(InstructionData)

#define TEST_DECODE(name, data, result)                                                            \
    QTest::newRow(name) << InstructionData{ data } << QString(result);


void CapstonePPCDecoderTest::initTestCase()
{
    m_project.loadPlugins();

    Plugin *plugin = m_project.getPluginManager()->getPluginByName("Capstone PPC decoder plugin");
    QVERIFY(plugin != nullptr);
    m_decoder = plugin->getIfc<IDecoder>();
    QVERIFY(m_decoder != nullptr);
}


void CapstonePPCDecoderTest::testInstructions()
{
    QFETCH(InstructionData, insnData);
    QFETCH(QString, expectedResult);

    MachineInstruction insn;
    LiftedInstruction result;
    Address sourceAddr = Address(0x1000);
    ptrdiff_t diff     = (HostAddress(&insnData) - sourceAddr).value();

    QVERIFY(m_decoder->disassembleInstruction(sourceAddr, diff, insn));
    QVERIFY(m_decoder->liftInstruction(insn, result));

    result.getFirstRTL()->simplify();
    QCOMPARE(result.getFirstRTL()->toString(), expectedResult);
}


void CapstonePPCDecoderTest::testInstructions_data()
{
    QTest::addColumn<InstructionData>("insnData");
    QTest::addColumn<QString>("expectedResult");

    // instructions listed alphabetically

    TEST_DECODE("add r0, r1, r2", "\x7c\x01\x12\x14",
                "0x00001000    0 *32* r0 := r1 + r2\n"
    );

    TEST_DECODE("add. r0, r1, r2", "\x7c\x01\x12\x15",
                "0x00001000    0 *32* r0 := r1 + r2\n"
                "              0 *v* %flags := SETFLAGS0( r0 )\n"
    );

    // TODO: addo (Not yet supported by Capstone)

    // TODO: addo. (Not yet supported by Capstone)

    TEST_DECODE("addc r0, r1, r2", "\x7c\x01\x10\x14",
                "0x00001000    0 *32* r0 := r1 + r2\n"
                "              0 *v* %flags := ADDFLAGSX( r0, r1, r2 )\n"
    );

    TEST_DECODE("addc. r0, r1, r2", "\x7c\x01\x10\x15",
                "0x00001000    0 *32* r0 := r1 + r2\n"
                "              0 *v* %flags := ADDFLAGSX0( r0, r1, r2 )\n"
    );

    // TODO: addco (Not yet supported by Capstone)

    // TODO: addco. (Not yet supported by Capstone)

    TEST_DECODE("adde r0, r1, r2", "\x7c\x01\x11\x14",
                "0x00001000    0 *32* r0 := r1 + (r2 + r202)\n"
    );

    TEST_DECODE("adde. r0, r1, r2", "\x7c\x01\x11\x15",
                "0x00001000    0 *32* r0 := r1 + (r2 + r202)\n"
                "              0 *v* %flags := SETFLAGS0( r0 )\n"
    );

    TEST_DECODE("addi r0, r1, -10", "\x38\x01\xff\xf6",
                "0x00001000    0 *32* r0 := r1 - 10\n"
    );

    TEST_DECODE("addic r0, r1, -10", "\x30\x01\xff\xf6",
                "0x00001000    0 *32* r0 := r1 - 10\n"
                "              0 *v* %flags := ADDFLAGSX( r0, r1, -10 )\n"
    );

    TEST_DECODE("addic. r0, r1, -10", "\x34\x01\xff\xf6",
                "0x00001000    0 *32* r0 := r1 - 10\n"
                "              0 *v* %flags := ADDFLAGSX0( r0, r1, -10 )\n"
    );

    TEST_DECODE("addis r0, r1, -10", "\x3c\x01\xff\xf6",
                "0x00001000    0 *32* r0 := r1 - 0xa0000\n"
    );

    TEST_DECODE("addme r0, r1", "\x7c\x01\x01\xd4",
                "0x00001000    0 *32* r0 := (r1 + r202) - 1\n"
    );

    TEST_DECODE("addme. r0, r1", "\x7c\x01\x01\xd5",
                "0x00001000    0 *32* r0 := (r1 + r202) - 1\n"
                "              0 *v* %flags := SETFLAGS0( r0 )\n"
    );

    // TODO: addmeo (Not yet supported by Capstone)

    // TODO: addmeo. (Not yet supported by Capstone)

    // TODO: addme64 (Not yet supported by Capstone)

    // TODO: addme64o (Not yet supported by Capstone)

    TEST_DECODE("addze r0, r1", "\x7c\x01\x01\x94",
                "0x00001000    0 *32* r0 := r1 + r202\n"
    );

    TEST_DECODE("addze. r0, r1", "\x7c\x01\x01\x95",
                "0x00001000    0 *32* r0 := r1 + r202\n"
                "              0 *v* %flags := SETFLAGS0( r0 )\n"
    );

    // TODO: addzeo (Not yet supported by Capstone)

    // TODO: addzeo. (Not yet supported by Capstone)

    // TODO: addze64 (Not yet supported by Capstone)

    // TODO: addze64o (Not yet supported by Capstone)

    TEST_DECODE("and r0, r1, r2", "\x7c\x20\x10\x38",
                "0x00001000    0 *32* r0 := r1 & r2\n"
    );

    TEST_DECODE("and. r0, r1, r2", "\x7c\x20\x10\x39",
                "0x00001000    0 *32* r0 := r1 & r2\n"
                "              0 *v* %flags := SETFLAGS0( r0 )\n"
    );

    TEST_DECODE("andi. r0, r1, 10", "\x70\x20\x00\x0a",
                "0x00001000    0 *32* r0 := r1 & 10\n"
                "              0 *v* %flags := SETFLAGS0( r0 )\n"
    );

    TEST_DECODE("andis. r0, r1, 10", "\x74\x20\x00\x0a",
                "0x00001000    0 *32* r0 := r1 & 0xa0000\n"
                "              0 *v* %flags := SETFLAGS0( r0 )\n"
    );

    TEST_DECODE("andc r0, r1, r2", "\x7c\x20\x10\x78",
                "0x00001000    0 *32* r0 := r1 & ~r2\n"
    );

    TEST_DECODE("andc. r0, r1, r2", "\x7c\x20\x10\x79",
                "0x00001000    0 *32* r0 := r1 & ~r2\n"
                "              0 *v* %flags := SETFLAGS0( r0 )\n"
    );

    TEST_DECODE("b 0x0800", "\x4b\xff\xf8\x00",
                "0x00001000    0 GOTO 0x00000800\n"
    );

    TEST_DECODE("b 0x2000", "\x48\x00\x10\x00",
                "0x00001000    0 GOTO 0x00002000\n"
    );

    TEST_DECODE("ba 0x2000", "\x48\x00\x20\x02",
                "0x00001000    0 GOTO 0x00002000\n"
    );

    TEST_DECODE("bl 0x2000", "\x48\x00\x10\x01",
                "0x00001000    0 *32* r300 := 0x1004\n"
                "              0 <all> := CALL 0x00002000(<all>)\n"
                "              Reaching definitions: <None>\n"
                "              Live variables: <None>\n"
    );

    TEST_DECODE("bla 0x2000", "\x48\x00\x20\x03",
                "0x00001000    0 *32* r300 := 0x1004\n"
                "              0 <all> := CALL 0x00002000(<all>)\n"
                "              Reaching definitions: <None>\n"
                "              Live variables: <None>\n"
    );

    // conditional branches (static)
    TEST_DECODE("blt 0x1000", "\x41\x80\x00\x00",
                "0x00001000    0 BRANCH 0x00001000, condition signed less\n"
                "High level: %flags\n"
    );

    TEST_DECODE("blt 0x2000", "\x41\x80\x10\x00",
                "0x00001000    0 BRANCH 0x00002000, condition signed less\n"
                "High level: %flags\n"
    );

    TEST_DECODE("blt cr5, 0x2000", "\x41\x94\x10\x00",
                "0x00001000    0 BRANCH 0x00002000, condition signed less\n"
                "High level: %flags\n"
    );

    TEST_DECODE("ble 0x1000", "\x40\x81\x00\x00",
                "0x00001000    0 BRANCH 0x00001000, condition signed less or equals\n"
                "High level: %flags\n"
    );

    TEST_DECODE("ble 0x2000", "\x40\x81\x10\x00",
                "0x00001000    0 BRANCH 0x00002000, condition signed less or equals\n"
                "High level: %flags\n"
    );

    TEST_DECODE("ble cr5, 0x2000", "\x40\x95\x10\x00",
                "0x00001000    0 BRANCH 0x00002000, condition signed less or equals\n"
                "High level: %flags\n"
    );

    TEST_DECODE("beq 0x1000", "\x41\x82\x00\x00",
                "0x00001000    0 BRANCH 0x00001000, condition equals\n"
                "High level: %flags\n"
    );

    TEST_DECODE("beq 0x2000", "\x41\x82\x10\x00",
                "0x00001000    0 BRANCH 0x00002000, condition equals\n"
                "High level: %flags\n"
    );

    TEST_DECODE("beq cr5, 0x2000", "\x41\x96\x10\x00",
                "0x00001000    0 BRANCH 0x00002000, condition equals\n"
                "High level: %flags\n"
    );

    TEST_DECODE("bge 0x1000", "\x40\x80\x00\x00",
                "0x00001000    0 BRANCH 0x00001000, condition signed greater or equals\n"
                "High level: %flags\n"
    );

    TEST_DECODE("bge 0x2000", "\x40\x80\x10\x00",
                "0x00001000    0 BRANCH 0x00002000, condition signed greater or equals\n"
                "High level: %flags\n"
    );

    TEST_DECODE("bge cr5, 0x2000", "\x40\x94\x10\x00",
                "0x00001000    0 BRANCH 0x00002000, condition signed greater or equals\n"
                "High level: %flags\n"
    );

    TEST_DECODE("bgt 0x1000", "\x41\x81\x00\x00",
                "0x00001000    0 BRANCH 0x00001000, condition signed greater\n"
                "High level: %flags\n"
    );

    TEST_DECODE("bgt 0x2000", "\x41\x81\x10\x00",
                "0x00001000    0 BRANCH 0x00002000, condition signed greater\n"
                "High level: %flags\n"
    );

    TEST_DECODE("bgt cr5, 0x2000", "\x41\x95\x10\x00",
                "0x00001000    0 BRANCH 0x00002000, condition signed greater\n"
                "High level: %flags\n"
    );

    TEST_DECODE("bnl 0x1000", "\x40\x80\x00\x00",
                "0x00001000    0 BRANCH 0x00001000, condition signed greater or equals\n"
                "High level: %flags\n"
    );

    TEST_DECODE("bnl 0x2000", "\x40\x80\x10\x00",
                "0x00001000    0 BRANCH 0x00002000, condition signed greater or equals\n"
                "High level: %flags\n"
    );

    TEST_DECODE("bnl cr5, 0x2000", "\x40\x94\x10\x00",
                "0x00001000    0 BRANCH 0x00002000, condition signed greater or equals\n"
                "High level: %flags\n"
    );

    TEST_DECODE("bne 0x1000", "\x40\x82\x00\x00",
                "0x00001000    0 BRANCH 0x00001000, condition not equals\n"
                "High level: %flags\n"
    );

    TEST_DECODE("bne 0x2000", "\x40\x82\x10\x00",
                "0x00001000    0 BRANCH 0x00002000, condition not equals\n"
                "High level: %flags\n"
    );

    TEST_DECODE("bne cr5, 0x2000", "\x40\x96\x10\x00",
                "0x00001000    0 BRANCH 0x00002000, condition not equals\n"
                "High level: %flags\n"
    );

    TEST_DECODE("bng 0x1000", "\x40\x81\x00\x00",
                "0x00001000    0 BRANCH 0x00001000, condition signed less or equals\n"
                "High level: %flags\n"
    );

    TEST_DECODE("bng 0x2000", "\x40\x81\x10\x00",
                "0x00001000    0 BRANCH 0x00002000, condition signed less or equals\n"
                "High level: %flags\n"
    );

    TEST_DECODE("bng cr5, 0x2000", "\x40\x95\x10\x00",
                "0x00001000    0 BRANCH 0x00002000, condition signed less or equals\n"
                "High level: %flags\n"
    );

    // TODO bso

    // TODO bns

    // TODO bun

    // TODO bnu

    TEST_DECODE("bctr", "\x4e\x80\x04\x20",
                "0x00001000    0 CASE [r301]\n"
    );

    TEST_DECODE("blr", "\x4e\x80\x00\x20",
                "0x00001000    0 RET\n"
                "              Modifieds: <None>\n"
                "              Reaching definitions: <None>\n"
    );

    TEST_DECODE("cmp 3, 0, 0, 1", "\x7d\x80\x08\x00",
                "0x00001000    0 *v* %flags := SUBFLAGSNS( r0, r1, r103 )\n"
    );

    TEST_DECODE("cmpi 0, 0, 0, -10", "\x2c\x00\xff\xf6",
                "0x00001000    0 *v* %flags := SUBFLAGSNS( r0, -10, 0 )\n"
    );

    TEST_DECODE("cmpi 3, 0, 0, -10", "\x2d\x80\xff\xf6",
                "0x00001000    0 *v* %flags := SUBFLAGSNS( r0, -10, r103 )\n"
    );

    TEST_DECODE("cmpl 3, 0, 0, 1", "\x7d\x80\x08\x40",
                "0x00001000    0 *v* %flags := SUBFLAGSNL( r0, r1, r103 )\n"
    );

    TEST_DECODE("cmpli 0, 0, 0, 1", "\x28\x00\x00\x01",
                "0x00001000    0 *v* %flags := SUBFLAGSNL( r0, 1, 0 )\n"
    );

    TEST_DECODE("cmpli 3, 0, 0, 1", "\x29\x80\x00\x01",
                "0x00001000    0 *v* %flags := SUBFLAGSNL( r0, 1, r103 )\n"
    );

    // TODO: Add semantics for cntlzw/cntlzw./cntlzd

    TEST_DECODE("crand 0, 2, 7", "\x4c\x02\x3a\x02",
                "0x00001000    0 *1* r99@[0:0] := (r99@[2:2]) & (r99@[7:7])\n"
    );

    TEST_DECODE("crandc 0, 2, 7", "\x4c\x02\x39\x02",
                "0x00001000    0 *1* r99@[0:0] := (r99@[2:2]) & ~(r99@[7:7])\n"
    );

    TEST_DECODE("creqv 0, 2, 7", "\x4c\x02\x3a\x42",
                "0x00001000    0 *1* r99@[0:0] := ~((r99@[2:2]) ^ (r99@[7:7]))\n"
    );

    TEST_DECODE("crnand 0, 2, 7", "\x4c\x02\x39\xc2",
                "0x00001000    0 *1* r99@[0:0] := ~(r99@[2:2]) | ~(r99@[7:7])\n"
    );

    TEST_DECODE("crnor 0, 2, 7", "\x4c\x02\x38\x42",
                "0x00001000    0 *1* r99@[0:0] := ~(r99@[2:2]) & ~(r99@[7:7])\n"
    );

    TEST_DECODE("cror 0, 2, 7", "\x4c\x02\x3b\x82",
                "0x00001000    0 *1* r99@[0:0] := (r99@[2:2]) | (r99@[7:7])\n"
    );

    TEST_DECODE("crorc 0, 2, 7", "\x4c\x02\x3b\x42",
                "0x00001000    0 *1* r99@[0:0] := (r99@[2:2]) | ~(r99@[7:7])\n"
    );

    TEST_DECODE("crxor 0, 2, 7", "\x4c\x02\x39\x82",
                "0x00001000    0 *1* r99@[0:0] := (r99@[2:2]) ^ (r99@[7:7])\n"
    );

    // TODO dcb*

    TEST_DECODE("divd 0, 1, 2", "\x7c\x01\x13\xd2",
                "0x00001000    0 *64* r0 := r1 / r2\n"
    );

    // TODO: divdo (Not yet supported by Capstone)

    TEST_DECODE("divdu 0, 1, 2", "\x7c\x01\x13\x92",
                "0x00001000    0 *64* r0 := r1 / r2\n"
    );

    // TODO: dvdou (Not yet supported by Capstone)

    TEST_DECODE("divw 0, 1, 2", "\x7c\x01\x13\xd6",
                "0x00001000    0 *32* r0 := r1 / r2\n"
    );

    TEST_DECODE("divw. 0, 1, 2", "\x7c\x01\x13\xd7",
                "0x00001000    0 *32* r0 := r1 / r2\n"
                "              0 *v* %flags := SETFLAGS0( r0 )\n"
    );

    // TODO: divwo (Not yet supported by Capstone)

    // TODO: divwo. (Not yet supported by Capstone)

    TEST_DECODE("divwu 0, 1, 2", "\x7c\x01\x13\x96",
                "0x00001000    0 *32* r0 := r1 / r2\n"
    );

    TEST_DECODE("divwu. 0, 1, 2", "\x7c\x01\x13\x97",
                "0x00001000    0 *32* r0 := r1 / r2\n"
                "              0 *v* %flags := SETFLAGS0( r0 )\n"
    );

    // TODO: divwuo (Not yet supported by Capstone)

    // TODO: divwuo. (Not yet supported by Capstone)

    TEST_DECODE("eqv 0, 1, 2", "\x7c\x20\x12\x38",
                "0x00001000    0 *32* r0 := ~(r1 ^ r2)\n"
    );

    TEST_DECODE("eqv. 0, 1, 2", "\x7c\x20\x12\x39",
                "0x00001000    0 *32* r0 := ~(r1 ^ r2)\n"
                "              0 *v* %flags := SETFLAGS0( r0 )\n"
    );

    TEST_DECODE("extsb 0, 1", "\x7c\x20\x07\x74",
                "0x00001000    0 *32* r0 := sgnex(8, 32, r1)\n"
    );

    TEST_DECODE("extsb. 0, 1", "\x7c\x20\x07\x75",
                "0x00001000    0 *32* r0 := sgnex(8, 32, r1)\n"
                "              0 *v* %flags := SETFLAGS0( r0 )\n");

    TEST_DECODE("extsh 0, 1", "\x7c\x20\x07\x34",
                "0x00001000    0 *32* r0 := sgnex(16, 32, r1)\n"
    );

    TEST_DECODE("extsh. 0, 1", "\x7c\x20\x07\x35",
                "0x00001000    0 *32* r0 := sgnex(16, 32, r1)\n"
                "              0 *v* %flags := SETFLAGS0( r0 )\n"
    );

    // TODO extsw

    TEST_DECODE("fabs 1, 2", "\xfc\x20\x12\x10",
                "0x00001000    0 *64* r33 := fabs(r34)\n"
    );

    TEST_DECODE("fabs. 1, 2", "\xfc\x20\x12\x11",
                "0x00001000    0 *64* r33 := fabs(r34)\n"
    );

    TEST_DECODE("fadd 1, 2, 3", "\xfc\x22\x18\x2a",
                "0x00001000    0 *64* r33 := r34 +f r35\n"
    );

    TEST_DECODE("fadd. 1, 2, 3", "\xfc\x22\x18\x2b",
                "0x00001000    0 *64* r33 := r34 +f r35\n"
    );

    TEST_DECODE("fadds 1, 2, 3", "\xec\x22\x18\x2a",
                "0x00001000    0 *64* r33 := r34 +f r35\n"
    );

    TEST_DECODE("fadds. 1, 2, 3", "\xec\x22\x18\x2b",
                "0x00001000    0 *64* r33 := r34 +f r35\n"
    );

    // TODO fcfid

    TEST_DECODE("fcmpu 1, 2, 3", "\xfc\x82\x18\x00",
                "0x00001000    0 *v* %flags := SETFFLAGSN( r34, r35, r101 )\n"
    );

    // TODO: fcmpo (Not yet supported by Capstone)

    // TODO fctid[z]

    TEST_DECODE("fctiw 3, 2", "\xfc\x60\x10\x1c",
                "0x00001000    0 *64* r35 := zfill(32, 64, ftoi(64, 32, r34))\n"
    );

    TEST_DECODE("fctiwz 3, 2", "\xfc\x60\x10\x1e",
                "0x00001000    0 *64* r35 := zfill(32, 64, ftoi(64, 32, r34))\n"
    );

    TEST_DECODE("fdiv 1, 2, 3", "\xfc\x22\x18\x24",
                "0x00001000    0 *64* r33 := r34 /f r35\n"
    );

    TEST_DECODE("fdiv. 1, 2, 3", "\xfc\x22\x18\x25",
                "0x00001000    0 *64* r33 := r34 /f r35\n"
    );

    TEST_DECODE("fdivs 1, 2, 3", "\xec\x22\x18\x24",
                "0x00001000    0 *64* r33 := r34 /f r35\n"
    );

    TEST_DECODE("fdivs. 1, 2, 3", "\xec\x22\x18\x25",
                "0x00001000    0 *64* r33 := r34 /f r35\n"
    );

    TEST_DECODE("fmadd 4, 2, 5, 1", "\xfc\x82\x09\x7a",
                "0x00001000    0 *64* r36 := (r34 *f r37) +f r33\n"
    );

    TEST_DECODE("fmadd. 4, 2, 5, 1", "\xfc\x82\x09\x7b",
                "0x00001000    0 *64* r36 := (r34 *f r37) +f r33\n"
    );

    TEST_DECODE("fmadds 4, 2, 5, 1", "\xec\x82\x09\x7a",
                "0x00001000    0 *64* r36 := (r34 *f r37) +f r33\n"
    );

    TEST_DECODE("fmadds. 4, 2, 5, 1", "\xec\x82\x09\x7b",
                "0x00001000    0 *64* r36 := (r34 *f r37) +f r33\n"
    );

    TEST_DECODE("fmr 3, 1", "\xfc\x60\x08\x90",
                "0x00001000    0 *64* r35 := r33\n"
    );

    TEST_DECODE("fmr. 3, 1", "\xfc\x60\x08\x91",
                "0x00001000    0 *64* r35 := r33\n"
    );

    TEST_DECODE("fmsub 4, 2, 5, 1", "\xfc\x82\x09\x78",
                "0x00001000    0 *64* r36 := (r34 *f r37) -f r33\n"
    );

    TEST_DECODE("fmsub. 4, 2, 5, 1", "\xfc\x82\x09\x79",
                "0x00001000    0 *64* r36 := (r34 *f r37) -f r33\n"
    );

    TEST_DECODE("fmsubs 4, 2, 5, 1", "\xec\x82\x09\x78",
                "0x00001000    0 *64* r36 := (r34 *f r37) -f r33\n"
    );

    TEST_DECODE("fmsubs. 4, 2, 5, 1", "\xec\x82\x09\x79",
                "0x00001000    0 *64* r36 := (r34 *f r37) -f r33\n"
    );

    TEST_DECODE("fmul 3, 1, 2", "\xfc\x61\x00\xb2",
                "0x00001000    0 *64* r35 := r33 *f r34\n"
    );

    TEST_DECODE("fmul. 3, 1, 2", "\xfc\x61\x00\xb3",
                "0x00001000    0 *64* r35 := r33 *f r34\n"
    );

    TEST_DECODE("fmuls 3, 1, 2", "\xec\x61\x00\xb2",
                "0x00001000    0 *64* r35 := r33 *f r34\n"
    );

    TEST_DECODE("fmuls. 3, 1, 2", "\xec\x61\x00\xb3",
                "0x00001000    0 *64* r35 := r33 *f r34\n"
    );

    TEST_DECODE("fnabs 4, 2", "\xfc\x80\x11\x10",
                "0x00001000    0 *64* r36 := -fabs(r34)\n"
    );

    TEST_DECODE("fnabs. 4, 2", "\xfc\x80\x11\x11",
                "0x00001000    0 *64* r36 := -fabs(r34)\n"
    );

    TEST_DECODE("fneg 3, 1",  "\xfc\x60\x08\x50",
                "0x00001000    0 *64* r35 := -r33\n"
    );

    TEST_DECODE("fneg. 3, 1", "\xfc\x60\x08\x51",
                "0x00001000    0 *64* r35 := -r33\n"
    );

    TEST_DECODE("fnmadd 4, 2, 5, 1", "\xfc\x82\x09\x7e",
                "0x00001000    0 *64* r36 := -(r34 *f r37) +f r33\n"
    );

    TEST_DECODE("fnmadd. 4, 2, 5, 1", "\xfc\x82\x09\x7f",
                "0x00001000    0 *64* r36 := -(r34 *f r37) +f r33\n"
    );

    TEST_DECODE("fnmadds 4, 2, 5, 1", "\xec\x82\x09\x7e",
                "0x00001000    0 *64* r36 := -(r34 *f r37) +f r33\n"
    );

    TEST_DECODE("fnmadds. 4, 2, 5, 1", "\xec\x82\x09\x7f",
                "0x00001000    0 *64* r36 := -(r34 *f r37) +f r33\n"
    );

    TEST_DECODE("fnmsub 4, 2, 5, 1", "\xfc\x82\x09\x7c",
                "0x00001000    0 *64* r36 := -(r34 *f r37) -f r33\n"
    );

    TEST_DECODE("fnmsub. 4, 2, 5, 1", "\xfc\x82\x09\x7d",
                "0x00001000    0 *64* r36 := -(r34 *f r37) -f r33\n"
    );

    TEST_DECODE("fnmsubs 4, 2, 5, 1", "\xec\x82\x09\x7c",
                "0x00001000    0 *64* r36 := -(r34 *f r37) -f r33\n"
    );

    TEST_DECODE("fnmsubs. 4, 2, 5, 1", "\xec\x82\x09\x7d",
                "0x00001000    0 *64* r36 := -(r34 *f r37) -f r33\n"
    );

    TEST_DECODE("fres 3, 2", "\xec\x60\x10\x30",
                "0x00001000    0 *32* r35 := 1 /f r34\n"
    );

    TEST_DECODE("fres. 3, 2", "\xec\x60\x10\x31",
                "0x00001000    0 *32* r35 := 1 /f r34\n"
    );

    TEST_DECODE("frsp 2, 8", "\xfc\x40\x40\x18",
                "0x00001000    0 *32* tmpf := fsize(64, 32, r40)\n"
                "              0 *64* r34 := fsize(32, 64, tmpf)\n"
    );

    TEST_DECODE("frsp. 2, 8", "\xfc\x40\x40\x19",
                "0x00001000    0 *32* tmpf := fsize(64, 32, r40)\n"
                "              0 *64* r34 := fsize(32, 64, tmpf)\n"
    );

    // TODO fsqrte[.]

    // TODO fsel[.]

    TEST_DECODE("fsqrt 4, 3", "\xfc\x80\x18\x2c",
                "0x00001000    0 *64* r36 := sqrt(r35)\n"
    );

    TEST_DECODE("fsqrt. 4, 3", "\xfc\x80\x18\x2d",
                "0x00001000    0 *64* r36 := sqrt(r35)\n"
    );

    TEST_DECODE("fsqrts 4, 3", "\xec\x80\x18\x2c",
                "0x00001000    0 *64* r36 := sqrt(r35)\n"
    );

    TEST_DECODE("fsqrts. 4, 3", "\xec\x80\x18\x2d",
                "0x00001000    0 *64* r36 := sqrt(r35)\n"
    );


    TEST_DECODE("fsub 3, 1, 2", "\xfc\x61\x10\x28",
                "0x00001000    0 *64* r35 := r33 -f r34\n"
    );

    TEST_DECODE("fsub. 3, 1, 2", "\xfc\x61\x10\x29",
                "0x00001000    0 *64* r35 := r33 -f r34\n"
    );

    TEST_DECODE("fsubs 3, 1, 2", "\xec\x61\x10\x28",
                "0x00001000    0 *64* r35 := r33 -f r34\n"
    );

    TEST_DECODE("fsubs. 3, 1, 2", "\xec\x61\x10\x29",
                "0x00001000    0 *64* r35 := r33 -f r34\n"
    );

    // Insn cache block instructions icb* TODO

    TEST_DECODE("lbz 3, 5(2)", "\x88\x62\x00\x05",
                "0x00001000    0 *32* r3 := zfill(8, 32, m[r2 + 5])\n");

    TEST_DECODE("lbzu 3, 4(2)", "\x8c\x62\x00\x04",
                "0x00001000    0 *32* r3 := zfill(8, 32, m[r2 + 4])\n"
                "              0 *32* r2 := r2 + 4\n"
    );

    // lbz[u]e TODO

    TEST_DECODE("lbzx 3, 1(2)", "\x7c\x61\x10\xae",
                "0x00001000    0 *32* r3 := zfill(8, 32, m[r1 + r2])\n");

    TEST_DECODE("lbzux 3, 1(2)", "\x7c\x61\x10\xee",
                "0x00001000    0 *32* r3 := zfill(8, 32, m[r1 + r2])\n"
                "              0 *32* r1 := r1 + r2\n"
    );

    // lbz[u]xe TODO


    // TODO ldarxe

    // TODO ld[u][x]e

    TEST_DECODE("lfd 3 1(2)", "\xc8\x62\x00\x01",
                "0x00001000    0 *64* r35 := m[r2 + 1]\n"
    );

    TEST_DECODE("lfdu 3, 1(2)", "\xcc\x62\x00\x04",
                "0x00001000    0 *64* r35 := m[r2 + 4]\n"
                "              0 *32* r2 := r2 + 4\n"
    );

    // TODO lfd[u]e

    TEST_DECODE("lfdx 3, 1(2)", "\x7c\x61\x14\xae",
                "0x00001000    0 *64* r35 := m[r1 + r2]\n"
    );

    TEST_DECODE("lfdux 3, 1(2)", "\x7c\x61\x14\xee",
                "0x00001000    0 *64* r35 := m[r1 + r2]\n"
                "              0 *32* r1 := r1 + r2\n"
    );

    // TODO lfd[u]xe

    TEST_DECODE("lfs 3, 1(2)", "\xc0\x62\x00\x01",
                "0x00001000    0 *32* r35 := fsize(32, 64, m[r2 + 1])\n"
    );

    TEST_DECODE("lfsu 3, 1(2)", "\xc4\x62\x00\x01",
                "0x00001000    0 *32* r35 := fsize(32, 64, m[r2 + 1])\n"
                "              0 *32* r2 := r2 + 1\n"
    );

    // TODO lfs[u]e

    TEST_DECODE("lfsx 3, 1(2)", "\x7c\x61\x14\x2e",
                "0x00001000    0 *32* r35 := fsize(32, 64, m[r1 + r2])\n"
    );

    TEST_DECODE("lfsux 3, 1(2)", "\x7c\x61\x14\x6e",
                "0x00001000    0 *32* r35 := fsize(32, 64, m[r1 + r2])\n"
                "              0 *32* r1 := r1 + r2\n"
    );

    // TODO lfs[u]xe

    TEST_DECODE("lha 3, 1(2)", "\xa8\x62\x00\x01",
                "0x00001000    0 *32* r3 := sgnex(16, 32, m[r2 + 1])\n"
    );

    TEST_DECODE("lhau 3, 1(2)", "\xac\x62\x00\x01",
                "0x00001000    0 *32* r3 := sgnex(16, 32, m[r2 + 1])\n"
                "              0 *32* r2 := r2 + 1\n"
    );

    // TODO lha[u]e

    TEST_DECODE("lhax 3, 1(2)", "\x7c\x61\x12\xae",
                "0x00001000    0 *32* r3 := m[r1 + r2]\n"
    );

    TEST_DECODE("lhaux 3, 1(2)", "\x7c\x61\x12\xee",
                "0x00001000    0 *32* r3 := m[r1 + r2]\n"
                "              0 *32* r1 := r1 + r2\n"
    );

    // TODO lha[u]xe

    TEST_DECODE("lhbrx 3, 4, 2", "\x7c\x64\x16\x2c",
                "0x00001000    0 *16* tmp1 := m[r4 + r2]\n"
                "              0 *32* r3 := (tmp1@[8:15]) | ((tmp1@[0:7]) << 8)\n"
    );

    // TODO lhbrxe

    TEST_DECODE("lhz 3, 1(2)", "\xa0\x62\x00\x01",
                "0x00001000    0 *32* r3 := zfill(16, 32, m[r2 + 1])\n"
    );

    TEST_DECODE("lhzu 3, 1(2)", "\xa4\x62\x00\x01",
                "0x00001000    0 *32* r3 := zfill(16, 32, m[r2 + 1])\n"
                "              0 *32* r2 := r2 + 1\n"
    );

    // TODO lhz[u]e

    TEST_DECODE("lhzx 3, 1(2)", "\x7c\x61\x12\x2e",
                "0x00001000    0 *32* r3 := zfill(16, 32, m[r1 + r2])\n"
    );

    TEST_DECODE("lhzux 3, 1(2)", "\x7c\x61\x12\x6e",
                "0x00001000    0 *32* r3 := zfill(16, 32, m[r1 + r2])\n"
                "              0 *32* r1 := r1 + r2\n"
    );

    // TODO lhz[u]xe

    TEST_DECODE("lmw 30, 4(2)", "\xbb\xc2\x00\x04",
                "0x00001000    0 *32* r30 := m[r2 + 4]\n"
                "              0 *32* r31 := m[r2 + 8]\n"
    );

    // TODO lswi/lswx

    // TODO lwarx[e]

    // TODO lwbrx[e]

    TEST_DECODE("lwz 3, 1(2)", "\x80\x62\x00\x01",
                "0x00001000    0 *32* r3 := m[r2 + 1]\n"
    );


    TEST_DECODE("lwzu 3, 1(2)", "\x84\x62\x00\x01",
                "0x00001000    0 *32* r3 := m[r2 + 1]\n"
                "              0 *32* r2 := r2 + 1\n"
    );

    // TODO lwz[u]e

    TEST_DECODE("lwzx 3, 1(2)", "\x7c\x61\x10\x2e",
                "0x00001000    0 *32* r3 := m[r1 + r2]\n"
    );

    TEST_DECODE("lwzux 3, 1(2)", "\x7c\x61\x10\x6e",
                "0x00001000    0 *32* r3 := m[r1 + r2]\n"
                "              0 *32* r1 := r1 + r2\n"
    );

    // TODO lwz[u]xe

    // TODO mbar

    TEST_DECODE("mcrf 2, 3", "\x4d\x0c\x00\x00",
                "0x00001000    0 *4* r102 := r103\n"
    );

    // TODO mcrfs (not yet supported by Capstone)

    // TODO mcrxr (not yet supported by Capstone)

    // TODO mcrxr64 (not yet supported by Capstone)

    // TODO mfapidi

    TEST_DECODE("mfcr 3", "\x7c\x60\x00\x26",
                "0x00001000    0 *32* r3 := (r100 << 28) + ((r101 << 24) + ((r102 << 20) + ((r103 << 16) + ((r104 << 12) + ((r105 << 8) + ((r106 << 4) + r107))))))\n"
    );

    // TODO mfdcr

    // TODO mffs/mffs.

    // TODO mfmsr

    // TODO mfspr

    // TODO msync

    TEST_DECODE("mtcrf 5, 7", "\x7c\xe0\x51\x20",
                "0x00001000    0 *4* r100 := r7@[0:3]\n"
                "              0 *4* r102 := r7@[8:11]\n"
    );

    // TODO mtdcr

    // TODO mtfsb[0|1][.]

    // TODO mtfsf

    // TODO mtfsfi[.]

    // TODO mtmsr

    // TODO mtspr

    // TODO mulhd[u]

    // TODO mulhw[.]

    // TODO mulld[o]

    TEST_DECODE("mulli 3, 1, 2", "\x1c\x61\x00\x02",
                "0x00001000    0 *32* r3 := r1 * 2\n"
    );

    TEST_DECODE("mulli 3, 1, -3", "\x1c\x61\xff\xfd",
                "0x00001000    0 *32* r3 := r1 * -3\n"
    );

    TEST_DECODE("mullw 3, 1, 2", "\x7c\x61\x11\xd6",
                "0x00001000    0 *32* r3 := r1 * r2\n"
    );

    TEST_DECODE("mullw. 3, 1, 2", "\x7c\x61\x11\xd7",
                "0x00001000    0 *32* r3 := r1 * r2\n"
                "              0 *v* %flags := SETFLAGS0( r3 )\n"
    );

    // TODO mullwo[.]

    TEST_DECODE("nand 9, 5, 1", "\x7c\xa9\x0b\xb8",
                "0x00001000    0 *32* r9 := ~r5 | ~r1\n"
    );

    TEST_DECODE("nand. 9, 5, 1", "\x7c\xa9\x0b\xb9",
                "0x00001000    0 *32* r9 := ~r5 | ~r1\n"
                "              0 *v* %flags := SETFLAGS0( r9 )\n"
    );

    TEST_DECODE("neg 5, 3", "\x7c\xa3\x00\xd0",
                "0x00001000    0 *32* r5 := 0 - r3\n"
    );

    TEST_DECODE("neg 5, 3", "\x7c\xa3\x00\xd1",
                "0x00001000    0 *32* r5 := 0 - r3\n"
                "              0 *v* %flags := SETFLAGS0( r5 )\n"
    );

    // TODO nego[.]

    TEST_DECODE("nor 8, 9, 4", "\x7d\x28\x20\xf8",
                "0x00001000    0 *32* r8 := ~r9 & ~r4\n"
    );

    TEST_DECODE("nor. 8, 9, 4", "\x7d\x28\x20\xf9",
                "0x00001000    0 *32* r8 := ~r9 & ~r4\n"
                "              0 *v* %flags := SETFLAGS0( r8 )\n"
    );

    TEST_DECODE("or 8, 9, 4", "\x7d\x28\x23\x78",
                "0x00001000    0 *32* r8 := r9 | r4\n"
    );

    TEST_DECODE("or. 8, 9, 4", "\x7d\x28\x23\x79",
                "0x00001000    0 *32* r8 := r9 | r4\n"
                "              0 *v* %flags := SETFLAGS0( r8 )\n"
    );

    TEST_DECODE("ori 8, 9, 0x10", "\x61\x28\x00\x10",
                "0x00001000    0 *32* r8 := r9 | 16\n"
    );

    TEST_DECODE("oris 8, 9, 0x10", "\x65\x28\x00\x10",
                "0x00001000    0 *32* r8 := r9 | 0x100000\n"
    );

    // TODO rfci

    // TODO rfi

    // TODO rld[i]cl

    // TODO rldicr

    // TODO rldic

    // TODO rldimi

    TEST_DECODE("rlwimi 3, 1, 2, 5, 6", "\x50\x23\x11\x4c",
                "0x00001000    0 *32* tmp_mask := 0x6000000\n"
                "              0 *32* r3 := ((r1 rl 2) & tmp_mask) | (r3 & ~tmp_mask)\n"
    );

    TEST_DECODE("rlwimi. 3, 1, 2, 5, 6", "\x50\x23\x11\x4d",
                "0x00001000    0 *32* tmp_mask := 0x6000000\n"
                "              0 *32* r3 := ((r1 rl 2) & tmp_mask) | (r3 & ~tmp_mask)\n"
                "              0 *v* %flags := SETFLAGS0( r3 )\n"
    );

    TEST_DECODE("rlwinm 3, 1, 2, 5, 6", "\x54\x23\x11\x4c",
                "0x00001000    0 *32* r3 := (r1 rl 2) & 0x6000000\n"
    );

    TEST_DECODE("rlwinm 3, 1, 2, 5, 6", "\x54\x23\x11\x4d",
                "0x00001000    0 *32* r3 := (r1 rl 2) & 0x6000000\n"
                "              0 *v* %flags := SETFLAGS0( r3 )\n"
    );

    TEST_DECODE("rlwnm 3, 1, 2, 5, 6", "\x5c\x23\x11\x4c",
                "0x00001000    0 *32* r3 := (r1 rl r2) & 0x6000000\n"
    );

    TEST_DECODE("rlwnm. 3, 1, 2, 5, 6", "\x5c\x23\x11\x4d",
                "0x00001000    0 *32* r3 := (r1 rl r2) & 0x6000000\n"
                "              0 *v* %flags := SETFLAGS0( r3 )\n"
    );

    // TODO sc

    // TODO sld

    TEST_DECODE("slw 3, 1, 2", "\x7c\x23\x10\x30",
                "0x00001000    0 *32* r3 := r1 << r2\n"
    );

    TEST_DECODE("slw. 3, 1, 2", "\x7c\x23\x10\x31",
                "0x00001000    0 *32* r3 := r1 << r2\n"
                "              0 *v* %flags := SETFLAGS0( r3 )\n"
    );

    // TODO srad[i]

    TEST_DECODE("sraw 3, 1, 2", "\x7c\x23\x16\x30",
                "0x00001000    0 *32* r3 := r1 >>A r2\n"
    );

    TEST_DECODE("sraw. 3, 1, 2", "\x7c\x23\x16\x31",
                "0x00001000    0 *32* r3 := r1 >>A r2\n"
                "              0 *v* %flags := SETFLAGS0( r3 )\n"
    );

    TEST_DECODE("srawi 3, 1, 2", "\x7c\x23\x16\x70",
                "0x00001000    0 *32* r3 := r1 >>A 2\n"
    );

    TEST_DECODE("srawi. 3, 1, 2", "\x7c\x23\x16\x71",
                "0x00001000    0 *32* r3 := r1 >>A 2\n"
                "              0 *v* %flags := SETFLAGS0( r3 )\n"
    );

    // TODO srd

    TEST_DECODE("srw 3, 1, 2", "\x7c\x23\x14\x30",
                "0x00001000    0 *32* r3 := r1 >> r2\n"
    );

    TEST_DECODE("srw. 3, 1, 2", "\x7c\x23\x14\x31",
                "0x00001000    0 *32* r3 := r1 >> r2\n"
                "              0 *v* %flags := SETFLAGS0( r3 )\n"
    );

    TEST_DECODE("stb 3, 1(2)", "\x98\x62\x00\x01",
                "0x00001000    0 *8* m[r2 + 1] := truncs(32, 8, r3)\n"
    );

    TEST_DECODE("stbu 3, 1(2)", "\x9c\x62\x00\x01",
                "0x00001000    0 *8* m[r2 + 1] := truncs(32, 8, r3)\n"
                "              0 *32* r3 := r2 + 1\n"
    );

    // TODO stb[u]e

    TEST_DECODE("stbx 3, 1(2)", "\x7c\x61\x11\xae",
                "0x00001000    0 *8* m[r1 + r2] := truncs(32, 8, r3)\n"
    );

    TEST_DECODE("stbux 3, 1(2)", "\x7c\x61\x11\xee",
                "0x00001000    0 *8* m[r1 + r2] := truncs(32, 8, r3)\n"
                "              0 *32* r1 := r1 + r2\n"
    );

    // TODO stb[u]xe


    // TODO stdcxe

    // TODO std[u]e

    // TODO std[u]xe

    TEST_DECODE("stfd 3, 1(2)", "\xd8\x62\x00\x01",
                "0x00001000    0 *64* m[r2 + 1] := r35\n"
    );

    TEST_DECODE("stfdu 3, 1(2)", "\xdc\x62\x00\x01",
                "0x00001000    0 *64* m[r2 + 1] := r35\n"
                "              0 *32* r2 := r2 + 1\n"
    );

    // TODO stfd[u]e

    TEST_DECODE("stfdx 3, 1(2)", "\x7c\x61\x15\xae",
                "0x00001000    0 *64* m[r1 + r2] := r35\n"
    );

    TEST_DECODE("stfdux 3, 1(2)", "\x7c\x61\x15\xee",
                "0x00001000    0 *64* m[r1 + r2] := r35\n"
                "              0 *32* r1 := r1 + r2\n"
    );

    // TODO stfd[u]xe

    // TODO stfiwx[e]

    TEST_DECODE("stfs 3, 1(2)", "\xd0\x62\x00\x01",
                "0x00001000    0 *32* m[r2 + 1] := fsize(64, 32, r35)\n"
    );

    TEST_DECODE("stfsu 3, 1(2)", "\xd4\x62\x00\x01",
                "0x00001000    0 *32* m[r2 + 1] := fsize(64, 32, r35)\n"
                "              0 *32* r2 := r2 + 1\n"
    );

    // TODO stfs[u]e

    TEST_DECODE("stfsx 3, 1(2)", "\x7c\x61\x15\x2e",
                "0x00001000    0 *32* m[r1 + r2] := fsize(64, 32, r35)\n"
    );

    TEST_DECODE("stfsux 3, 1(2)", "\x7c\x61\x15\x6e",
                "0x00001000    0 *32* m[r1 + r2] := fsize(64, 32, r35)\n"
                "              0 *32* r1 := r1 + r2\n"
    );

    // TODO stfs[u]xe

    TEST_DECODE("sth 3, 1(2)", "\xb0\x62\x00\x01",
                "0x00001000    0 *16* m[r2 + 1] := truncs(32, 16, r3)\n"
    );

    TEST_DECODE("sthu 3, 1(2)", "\xb4\x62\x00\x01",
                "0x00001000    0 *16* m[r2 + 1] := truncs(32, 16, r3)\n"
                "              0 *32* r3 := r2 + 1\n"
    );

    // TODO sth[u]e

    TEST_DECODE("sthx 3, 1(2)", "\x7c\x61\x13\x2e",
                "0x00001000    0 *16* m[r1 + r2] := truncs(32, 16, r3)\n"
    );

    TEST_DECODE("sthux 3, 1(2)", "\x7c\x61\x13\x6e",
                "0x00001000    0 *16* m[r1 + r2] := truncs(32, 16, r3)\n"
                "              0 *32* r1 := r1 + r2\n"
    );

    // TODO sth[u]xe

    TEST_DECODE("sthbrx 3, 1, 2", "\x7c\x61\x17\x2c",
                "0x00001000    0 *16* m[r1 + r2] := ((r3@[0:7]) << 8) | (r3@[8:15])\n"
    );

    // TODO sthbrxe

    TEST_DECODE("stmw 30, 4(2)", "\xbf\xc2\x00\x04",
                "0x00001000    0 *32* m[r2 + 4] := r30\n"
                "              0 *32* m[r2 + 8] := r31\n");

    // TODO stswi

    // TODO stswx

    TEST_DECODE("stw 3, 1(2)", "\x90\x62\x00\x01",
                "0x00001000    0 *32* m[r2 + 1] := r3\n"
    );

    TEST_DECODE("stwu 3, 1(2)", "\x94\x62\x00\x01",
                "0x00001000    0 *32* m[r2 + 1] := r3\n"
                "              0 *32* r3 := r2 + 1\n"
    );

    // TODO stw[u]e

    TEST_DECODE("stwx 3, 1(2)", "\x7c\x61\x11\x2e",
                "0x00001000    0 *32* m[r1 + r2] := r3\n"
    );

    TEST_DECODE("stwux 3, 1(2)", "\x7c\x61\x11\x6e",
                "0x00001000    0 *32* m[r1 + r2] := r3\n"
                "              0 *32* r1 := r1 + r2\n");

    // TODO stw[u]xe

    // TODO stwbrx[e]

    // TODO stwcx[e].

    TEST_DECODE("subf 3, 1, 2", "\x7c\x61\x10\x50",
                "0x00001000    0 *32* r3 := r2 - r1\n"
    );

    TEST_DECODE("subf. 3, 1, 2", "\x7c\x61\x10\x51",
                "0x00001000    0 *32* r3 := r2 - r1\n"
                "              0 *v* %flags := SETFLAGS0( r3 )\n");

    // TODO subfo[.]

    TEST_DECODE("subfc 3, 1, 2", "\x7c\x61\x10\x10",
                "0x00001000    0 *32* r3 := r2 - r1\n"
                "              0 *v* %flags := SUBFLAGSX( r3, r2, r1 )\n");

    TEST_DECODE("subfc. 3, 1, 2", "\x7c\x61\x10\x11",
                "0x00001000    0 *32* r3 := r2 - r1\n"
                "              0 *v* %flags := SUBFLAGSX0( r3, r2, r1 )\n");

    // TODO subfco[.]

    // TODO subfe*

    TEST_DECODE("subfic 3, 1, 2", "\x20\x61\x00\x02",
                "0x00001000    0 *32* r3 := 2 - r1\n"
                "              0 *v* %flags := SUBFLAGSX( r3, 2, r1 )\n");

    // TODO subfme*

    TEST_DECODE("subfze 3, 1", "\x7c\x61\x01\x90",
                "0x00001000    0 *32* r3 := ~r1 + r202\n"
    );

    TEST_DECODE("subfze. 3, 1", "\x7c\x61\x01\x91",
                "0x00001000    0 *32* r3 := ~r1 + r202\n"
                "              0 *v* %flags := SUBFLAGS0( r3 )\n");

    // TODO subfzeo[.]

    // TODO subfze64[o]

    // TODO td[i]

    // TODO tlb*

    // TODO tw[i]

    // TODO wrtee[i]

    TEST_DECODE("xor 3, 1, 2", "\x7c\x23\x12\x78",
                "0x00001000    0 *32* r3 := r1 ^ r2\n"
    );

    TEST_DECODE("xor. 3, 1, 2", "\x7c\x23\x12\x79",
                "0x00001000    0 *32* r3 := r1 ^ r2\n"
                "              0 *v* %flags := SETFLAGS0( r3 )\n"
    );

    TEST_DECODE("xori 2, 3, 0x40", "\x68\x62\x00\x40",
                "0x00001000    0 *32* r2 := r3 ^ 64\n"
    );

    TEST_DECODE("xoris 2, 3, 1", "\x6c\x62\x00\x01",
                "0x00001000    0 *32* r2 := r3 ^ 0x10000\n"
    );
}


QTEST_GUILESS_MAIN(CapstonePPCDecoderTest)

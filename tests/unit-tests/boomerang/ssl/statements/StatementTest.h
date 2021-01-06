#pragma region License
/*
 * This file is part of the Boomerang Decompiler.
 *
 * See the file "LICENSE.TERMS" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 */
#pragma endregion License
#pragma once


#include "TestUtils.h"


/**
 * Provides the interface for the StatementTest class,
 * which tests the dataflow subsystems
 */
class StatementTest : public BoomerangTestWithPlugins
{
    Q_OBJECT

private slots:
    void testFragment();
    void testIsNull();
    void testCanPropagateToExp();
    void testCanPropagateToExp_data();
    void testPropagateToThis();
    void testPropagateFlagsToThis();

    /// Test assignment test
    void testIsAssign();

    /// Test the isFlagAssgn function, and opFlagCall
    void testIsFlagAssgn();

    /// Test the finding of locations used by this statement
    void testAddUsedLocsAssign();
    void testAddUsedLocsBranch();
    void testAddUsedLocsCase();
    void testAddUsedLocsCall();
    void testAddUsedLocsReturn();
    void testAddUsedLocsBool();

    /// Test the visitor code that fixes references that were to locations defined by calls
    void testBypass();
};

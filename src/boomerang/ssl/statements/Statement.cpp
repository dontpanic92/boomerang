#pragma region License
/*
 * This file is part of the Boomerang Decompiler.
 *
 * See the file "LICENSE.TERMS" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 */
#pragma endregion License
#include "Statement.h"

#include "boomerang/core/Project.h"
#include "boomerang/core/Settings.h"
#include "boomerang/db/Prog.h"
#include "boomerang/db/proc/UserProc.h"
#include "boomerang/ssl/exp/Binary.h"
#include "boomerang/ssl/exp/Const.h"
#include "boomerang/ssl/exp/Location.h"
#include "boomerang/ssl/exp/RefExp.h"
#include "boomerang/ssl/statements/Assign.h"
#include "boomerang/ssl/statements/CallStatement.h"
#include "boomerang/util/log/Log.h"
#include "boomerang/visitor/expmodifier/CallBypasser.h"
#include "boomerang/visitor/expvisitor/UsedLocsFinder.h"
#include "boomerang/visitor/stmtexpvisitor/UsedLocsVisitor.h"
#include "boomerang/visitor/stmtmodifier/StmtPartModifier.h"


SharedStmt Statement::wild = SharedStmt(new Assign(Terminal::get(opNil), Terminal::get(opNil)));
static uint32 m_nextStmtID = 0;


Statement::Statement(StmtType kind)
    : m_fragment(nullptr)
    , m_proc(nullptr)
    , m_number(0)
    , m_kind(kind)
{
    m_id = m_nextStmtID++;
}


Statement::Statement(const Statement &other)
    : enable_shared_from_this(other)
    , m_fragment(other.m_fragment)
    , m_proc(other.m_proc)
    , m_number(other.m_number)
    , m_kind(other.m_kind)
{
    m_id = m_nextStmtID++;
}


Statement &Statement::operator=(const Statement &other)
{
    if (this == &other) {
        return *this;
    }

    m_fragment = other.m_fragment;
    m_proc     = other.m_proc;
    m_number   = other.m_number;
    m_id       = m_nextStmtID++;

    return *this;
}


bool Statement::operator==(const Statement &rhs) const
{
    return getID() == rhs.getID();
}


bool Statement::operator<(const Statement &rhs) const
{
    return getID() < rhs.getID();
}


void Statement::setProc(UserProc *proc)
{
    m_proc = proc;

    const bool assumeABICompliance = (proc && proc->getProg())
                                         ? proc->getProg()->getProject()->getSettings()->assumeABI
                                         : false;
    LocationSet exps, defs;
    addUsedLocs(exps);
    getDefinitions(defs, assumeABICompliance);
    exps.makeUnion(defs);

    for (SharedExp exp : exps) {
        if (exp->isLocation()) {
            exp->access<Location>()->setProc(proc);
        }
    }
}


void Statement::getDefinitions(LocationSet &, bool) const
{
}


bool Statement::definesLoc(SharedExp) const
{
    return false;
}


void Statement::simplifyAddr()
{
}


OStream &operator<<(OStream &os, const SharedStmt &s)
{
    if (s == nullptr) {
        os << "nullptr ";
        return os;
    }

    s->print(os);
    return os;
}


bool Statement::isFlagAssign() const
{
    return m_kind == StmtType::Assign &&
           static_cast<const Assign *>(this)->getRight()->isFlagCall();
}


QString Statement::toString() const
{
    QString tgt;
    OStream ost(&tgt);
    print(ost);
    return tgt;
}


bool Statement::canPropagateToExp(const Exp &exp)
{
    if (!exp.isSubscript()) {
        return false;
    }

    const RefExp &ref = static_cast<const RefExp &>(exp);

    if (ref.isImplicitDef()) {
        // Can't propagate statement "-" or "0" (implicit assignments)
        return false;
    }

    const SharedConstStmt def = ref.getDef();
    if (def->isNullStatement()) {
        // Don't propagate a null statement! Can happen with %pc's (would have no effect, and would
        // infinitely loop)
        return false;
    }

    if (!def->isAssign()) {
        return false; // Only propagate ordinary assignments (so far)
    }

    // Assigning to an array, don't propagate (Could be alias problems?)
    return !def->as<const Assign>()->getType()->isArray();
}


bool Statement::propagateToThis(int propMaxDepth, const ExpIntMap *destCounts, bool force)
{
    bool thisChange = false;
    int changes     = 0;

    do {
        // addUsedLocs(..,true) -> true to also add uses from collectors. For example, want to
        // propagate into the reaching definitions of calls. Third parameter is false to find
        // all locations, not just those inside m[...]
        LocationSet usedExps;
        addUsedLocs(usedExps, true, false);
        thisChange = false; // True if changed this iteration of the do/while loop

        // Example: m[r24{10}] := r25{20} + m[r26{30}]
        // exps has r24{10}, r25{20}, m[r26{30}], r26{30}
        for (SharedExp usedHere : usedExps) {
            if (!Statement::canPropagateToExp(*usedHere)) {
                continue;
            }

            assert(usedHere->access<RefExp>()->getDef()->isAssignment());
            std::shared_ptr<Assignment>
                def       = usedHere->access<RefExp>()->getDef()->as<Assignment>();
            SharedExp rhs = def->getRight();

            // Must never propagate unsubscripted memofs, or memofs that don't yet have symbols.
            // You could be propagating past a definition, thereby invalidating the IR.
            // If force is true, ignore the fact that a memof should not be propagated
            // (for switch analysis)
            if (rhs->containsBadMemof() && !(force && rhs->isMemOf())) {
                continue;
            }

            const SharedExp lhs = def->getLeft();

            // Check if the -l flag (propMaxDepth) prevents this propagation,
            // but always propagate to %flags
            if (!destCounts || lhs->isFlags() || def->getRight()->containsFlags()) {
                thisChange |= replaceRef(usedHere, def);
            }
            else {
                ExpIntMap::const_iterator ff = destCounts->find(usedHere);

                if (ff == destCounts->end()) {
                    thisChange |= replaceRef(usedHere, def);
                }
                else if (ff->second <= 1) {
                    thisChange |= replaceRef(usedHere, def);
                }
                else if (rhs->getComplexityDepth(m_proc) < propMaxDepth) {
                    thisChange |= replaceRef(usedHere, def);
                }
            }
        }
    } while (thisChange && ++changes < 10);

    // Simplify is very costly, especially for calls.
    // I hope that doing one simplify at the end will not affect any result...
    simplify();

    return changes > 0;
}


bool Statement::propagateFlagsToThis()
{
    bool thisChange = false;
    int changes     = 0;

    do {
        LocationSet usedExps;
        addUsedLocs(usedExps, true);

        for (SharedExp usedHere : usedExps) {
            if (!usedHere->isSubscript()) {
                continue; // e.g. %pc
            }

            const std::shared_ptr<Assignment> def = std::dynamic_pointer_cast<Assignment>(
                usedHere->access<RefExp>()->getDef());

            // process only if it has definition with rhs
            if (!def || !def->getRight()) {
                continue;
            }

            const SharedExp base = usedHere->access<Exp, 1>(); // Either RefExp or Location ?
            if (base->isFlags() || base->isMainFlag()) {
                thisChange |= replaceRef(usedHere, def);
            }
        }
    } while (thisChange && ++changes < 10);

    simplify();

    return changes > 0;
}


void Statement::setTypeForExp(SharedExp, SharedType)
{
    assert(false);
}


bool Statement::replaceRef(SharedExp e, const std::shared_ptr<Assignment> &def)
{
    SharedExp rhs = def->getRight();
    assert(rhs);

    SharedExp base = e->getSubExp1();
    // Could be propagating %flags into %CF
    SharedExp lhs = def->getLeft();

    /* When one of the main flags is used bare, and was defined via a flag function,
     * apply the semantics for it. For example, the x86 'sub lhs, rhs' instruction effectively
     * sets the CF flag to 'lhs <u rhs'.
     * Extract the arguments of the flag function call, and apply the semantics manually.
     * This should rather be done in the SSL file (not hard-coded), but doing this currently breaks
     * Type Analysis (because Type Analysis is also done for subexpressions of dead definitions).
     * Note: the flagcall is a binary, with a Const (the name), and a list of expressions. Example
     * for the 'SUBFLAGS' flag call:
     *
     *          defRhs
     *         /     \
     *     Const    opList
     * "SUBFLAGS"   /    \
     *             P1   opList
     *                  /    \
     *                 P2   opList
     *                      /    \
     *                     P3   opNil
     */
    if (lhs && lhs->isFlags()) {
        if (!rhs) {
            return false;
        }
        else if (rhs->isIntConst() && *lhs != *rhs) {
            // e.g. %flags := 0
            searchAndReplace(*e, rhs, true);
            return true;
        }
        else if (!rhs->isFlagCall()) {
            return false;
        }

        const QString flagFuncName = rhs->access<Const, 1>()->getStr();

        if (flagFuncName.startsWith("SUBFLAGSFL")) {
            switch (base->getOper()) {
            case opCF: {
                // for float cf we'll replace the CF with (P1<P2)
                SharedExp replacement = Binary::get(opLess, rhs->access<Exp, 2, 1>(),
                                                    rhs->access<Exp, 2, 2, 1>());
                searchAndReplace(*RefExp::get(Terminal::get(opCF), def), replacement, true);
                return true;
            }
            case opZF: {
                // for float zf we'll replace the ZF with (P1==P2)
                SharedExp replacement = Binary::get(opEquals, rhs->access<Exp, 2, 1>(),
                                                    rhs->access<Exp, 2, 2, 1>());
                searchAndReplace(*RefExp::get(Terminal::get(opZF), def), replacement, true);
                return true;
            }

            default: break;
            }
        }
        else if (flagFuncName.startsWith("SUBFLAGS")) {
            const SharedExp subLhs    = rhs->access<Exp, 2, 1>();
            const SharedExp subRhs    = rhs->access<Exp, 2, 2, 1>();
            const SharedExp subResult = rhs->access<Exp, 2, 2, 2, 1>();

            switch (base->getOper()) {
            case opCF: {
                const SharedExp replacement = Binary::get(opLessUns, subLhs, subRhs);
                searchAndReplace(*RefExp::get(Terminal::get(opCF), def), replacement, true);
                return true;
            }
            case opZF: {
                // for zf we only want to check if the result part of the subflags is equal to zero
                const SharedExp replacement = Binary::get(opEquals, subResult, Const::get(0));
                searchAndReplace(*RefExp::get(Terminal::get(opZF), def), replacement, true);
                return true;
            }
            case opNF: {
                // for sf we only want to check if the result part of the subflags is less than zero
                const SharedExp replacement = Binary::get(opLess, subResult, Const::get(0));
                searchAndReplace(*RefExp::get(Terminal::get(opNF), def), replacement, true);
                return true;
            }
            case opOF: {
                // (op1 < 0 && op2 >= 0 && result >= 0) || (op1 >= 0 && op2 < 0 && result < 0)
                const SharedExp replacement = Binary::get(
                    opOr,
                    Binary::get(opAnd,
                                Binary::get(opAnd, Binary::get(opLess, subLhs, Const::get(0)),
                                            Binary::get(opGtrEq, subRhs, Const::get(0))),
                                Binary::get(opGtrEq, subResult, Const::get(0))),
                    Binary::get(opAnd,
                                Binary::get(opAnd, Binary::get(opGtrEq, subLhs, Const::get(0)),
                                            Binary::get(opLess, subRhs, Const::get(0))),
                                Binary::get(opLess, subResult, Const::get(0))));
                searchAndReplace(*RefExp::get(Terminal::get(opOF), def), replacement, true);
                return true;
            }
            default: break;
            }
        }
        else if (flagFuncName.startsWith("LOGICALFLAGS")) {
            const SharedExp param = rhs->access<Exp, 2, 1>();
            switch (base->getOper()) {
            case opNF: {
                SharedExp replacement = Binary::get(opLess, param, Const::get(0));
                searchAndReplace(*RefExp::get(Terminal::get(opNF), def), replacement, true);
                return true;
            }
            case opZF: {
                SharedExp replacement = Binary::get(opEquals, param, Const::get(0));
                searchAndReplace(*RefExp::get(Terminal::get(opZF), def), replacement, true);
                return true;
            }
            case opCF: {
                SharedExp replacement = Const::get(0);
                searchAndReplace(*RefExp::get(Terminal::get(opCF), def), replacement, true);
                return true;
            }
            case opOF: {
                const SharedExp replacement = Const::get(0);
                searchAndReplace(*RefExp::get(Terminal::get(opOF), def), replacement, true);
                return true;
            }
            default: break;
            }
        }
        else if (flagFuncName.startsWith("INCDECFLAGS")) {
            const SharedExp param = rhs->access<Exp, 2, 1>();
            switch (base->getOper()) {
            case opOF: {
                const SharedExp replacement = Const::get(0);
                searchAndReplace(*RefExp::get(Terminal::get(opOF), def), replacement, true);
                return true;
            }
            case opZF: {
                const SharedExp replacement = Binary::get(opEquals, param, Const::get(0));
                searchAndReplace(*RefExp::get(Terminal::get(opZF), def), replacement, true);
                return true;
            }
            case opNF: {
                const SharedExp replacement = Binary::get(opLess, param, Const::get(0));
                searchAndReplace(*RefExp::get(Terminal::get(opNF), def), replacement, true);
                return true;
            }
            default: break;
            }
        }
    }

    // do the replacement; last parameter true to also change collectors
    return searchAndReplace(*e, rhs, true);
}


bool Statement::isNullStatement() const
{
    if (!this->isAssign()) {
        return false;
    }

    SharedExp right = static_cast<const Assign *>(this)->getRight();

    if (right->isSubscript()) {
        // Must refer to self to be null
        return right->access<RefExp>()->getDef().get() == this;
    }
    else {
        // Null if left == right
        return *shared_from_this()->as<const Assign>()->getLeft() == *right;
    }
}


void Statement::bypass()
{
    // Use the Part modifier so we don't change the top level of LHS of assigns etc
    CallBypasser cb(shared_from_this());
    StmtPartModifier sm(&cb);

    accept(&sm);

    if (cb.isTopChanged()) {
        simplify(); // E.g. m[esp{20}] := blah -> m[esp{-}-20+4] := blah
    }
}


void Statement::addUsedLocs(LocationSet &used, bool cc /* = false */, bool memOnly /*= false */)
{
    UsedLocsFinder ulf(used, memOnly);
    UsedLocsVisitor ulv(&ulf, cc);

    accept(&ulv);
}


SharedType Statement::meetWithFor(const SharedType &ty, const SharedExp &e, bool &changed)
{
    bool thisCh        = false;
    SharedType typeFor = getTypeForExp(e);

    assert(typeFor);
    SharedType newType = typeFor->meetWith(ty, thisCh);

    if (thisCh) {
        changed = true;
        setTypeForExp(e, newType->clone());
    }

    return newType;
}

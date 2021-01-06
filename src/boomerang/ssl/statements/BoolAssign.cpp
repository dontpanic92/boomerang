#pragma region License
/*
 * This file is part of the Boomerang Decompiler.
 *
 * See the file "LICENSE.TERMS" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 */
#pragma endregion License
#include "BoolAssign.h"

#include "boomerang/ifc/ICodeGenerator.h"
#include "boomerang/ssl/exp/Const.h"
#include "boomerang/ssl/exp/Terminal.h"
#include "boomerang/ssl/exp/Ternary.h"
#include "boomerang/ssl/statements/Assign.h"
#include "boomerang/ssl/statements/StatementHelper.h"
#include "boomerang/util/LocationSet.h"
#include "boomerang/visitor/expvisitor/ExpVisitor.h"
#include "boomerang/visitor/stmtexpvisitor/StmtExpVisitor.h"
#include "boomerang/visitor/stmtmodifier/StmtModifier.h"
#include "boomerang/visitor/stmtmodifier/StmtPartModifier.h"
#include "boomerang/visitor/stmtvisitor/StmtVisitor.h"


BoolAssign::BoolAssign(SharedExp lhs, BranchType bt, SharedExp cond)
    : Assignment(StmtType::BoolAssign, lhs)
    , m_jumpType(bt)
    , m_cond(cond)
    , m_isFloat(false)
{
    assert(m_cond != nullptr);
}


BoolAssign::BoolAssign(const BoolAssign &other)
    : Assignment(other)
    , m_jumpType(other.m_jumpType)
    , m_cond(other.m_cond->clone())
    , m_isFloat(other.m_isFloat)
{
}


BoolAssign::~BoolAssign()
{
}


BoolAssign &BoolAssign::operator=(const BoolAssign &other)
{
    Assignment::operator=(other);

    m_jumpType = other.m_jumpType;
    m_cond     = other.m_cond->clone();
    m_isFloat  = other.m_isFloat;

    return *this;
}


void BoolAssign::setCondType(BranchType cond, bool usesFloat /*= false*/)
{
    m_jumpType = cond;
    m_isFloat  = usesFloat;
    setCondExpr(Terminal::get(opFlags));
}


SharedExp BoolAssign::getCondExpr() const
{
    return m_cond;
}


void BoolAssign::setCondExpr(SharedExp pss)
{
    m_cond = pss;
    assert(m_cond != nullptr);
}


void BoolAssign::printCompact(OStream &os) const
{
    os << "BOOL ";
    m_lhs->print(os);
    os << " := CC(";

    switch (m_jumpType) {
    case BranchType::JE: os << "equals"; break;
    case BranchType::JNE: os << "not equals"; break;
    case BranchType::JSL: os << "signed less"; break;
    case BranchType::JSLE: os << "signed less or equals"; break;
    case BranchType::JSGE: os << "signed greater or equals"; break;
    case BranchType::JSG: os << "signed greater"; break;
    case BranchType::JUL: os << "unsigned less"; break;
    case BranchType::JULE: os << "unsigned less or equals"; break;
    case BranchType::JUGE: os << "unsigned greater or equals"; break;
    case BranchType::JUG: os << "unsigned greater"; break;
    case BranchType::JMI: os << "minus"; break;
    case BranchType::JPOS: os << "plus"; break;
    case BranchType::JOF: os << "overflow"; break;
    case BranchType::JNOF: os << "no overflow"; break;
    case BranchType::JPAR: os << "ev parity"; break;
    case BranchType::JNPAR: os << "odd parity"; break;
    case BranchType::INVALID: assert(false); break;
    }

    os << ')';

    if (m_isFloat) {
        os << ", float";
    }

    os << "\nHigh level: ";
    m_cond->print(os);
    os << "\n";
}


SharedStmt BoolAssign::clone() const
{
    return std::make_shared<BoolAssign>(*this);
}


bool BoolAssign::accept(StmtVisitor *visitor) const
{
    return visitor->visit(this);
}


void BoolAssign::simplify()
{
    condToRelational(m_cond, m_jumpType);
}


void BoolAssign::getDefinitions(LocationSet &defs, bool) const
{
    defs.insert(getLeft());
}


bool BoolAssign::search(const Exp &pattern, SharedExp &result) const
{
    assert(m_lhs != nullptr);
    assert(m_cond != nullptr);

    return m_lhs->search(pattern, result) || m_cond->search(pattern, result);
}


bool BoolAssign::searchAll(const Exp &pattern, std::list<SharedExp> &result) const
{
    assert(m_lhs != nullptr);
    assert(m_cond != nullptr);

    bool ch = false;

    ch |= m_lhs->searchAll(pattern, result);
    ch |= m_cond->searchAll(pattern, result);

    return ch;
}


bool BoolAssign::searchAndReplace(const Exp &pattern, SharedExp replace, bool)
{
    assert(m_lhs != nullptr);
    assert(m_cond != nullptr);

    bool chl = false, chr = false;
    m_lhs  = m_lhs->searchReplaceAll(pattern, replace, chl);
    m_cond = m_cond->searchReplaceAll(pattern, replace, chr);

    assert(m_lhs != nullptr);
    assert(m_cond != nullptr);

    return chl || chr;
}


SharedExp BoolAssign::getRight() const
{
    return m_cond;
}


bool BoolAssign::accept(StmtExpVisitor *v)
{
    bool visitChildren = true;
    bool ret           = v->visit(shared_from_this()->as<BoolAssign>(), visitChildren);

    if (!visitChildren) {
        return ret;
    }

    if (ret) {
        ret = m_cond->acceptVisitor(v->ev);
    }

    return ret;
}


bool BoolAssign::accept(StmtModifier *v)
{
    bool visitChildren = true;
    v->visit(shared_from_this()->as<BoolAssign>(), visitChildren);

    if (v->m_mod) {
        if (visitChildren) {
            m_cond = m_cond->acceptModifier(v->m_mod);
            assert(m_cond != nullptr);
        }

        if (visitChildren && m_lhs->isMemOf()) {
            m_lhs->setSubExp1(m_lhs->getSubExp1()->acceptModifier(v->m_mod));
        }
    }

    return true;
}


bool BoolAssign::accept(StmtPartModifier *v)
{
    bool visitChildren;

    v->visit(shared_from_this()->as<BoolAssign>(), visitChildren);

    if (visitChildren) {
        m_cond = m_cond->acceptModifier(v->mod);
        assert(m_cond != nullptr);
    }

    if (m_lhs && visitChildren) {
        m_lhs = m_lhs->acceptModifier(v->mod);
    }

    return true;
}

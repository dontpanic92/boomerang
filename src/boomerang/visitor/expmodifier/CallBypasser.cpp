#pragma region License
/*
 * This file is part of the Boomerang Decompiler.
 *
 * See the file "LICENSE.TERMS" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 */
#pragma endregion License
#include "CallBypasser.h"

#include "boomerang/ssl/exp/Location.h"
#include "boomerang/ssl/exp/RefExp.h"
#include "boomerang/ssl/statements/CallStatement.h"


CallBypasser::CallBypasser(const SharedStmt &enclosing)
    : m_enclosingStmt(enclosing)
{
}


SharedExp CallBypasser::postModify(const std::shared_ptr<RefExp> &exp)
{
    // Note: r (the pointer) will always == ret (also the pointer) here,
    // so the below is safe and avoids a cast
    SharedStmt def = exp->getDef();
    SharedExp ret  = exp;

    if (def && def->isCall()) {
        std::shared_ptr<CallStatement> call = def->as<CallStatement>();
        bool ch;

        assert(ret->isSubscript());
        ret = call->bypassRef(ret->access<RefExp>(), ch);

        if (ch) {
            m_unchanged &= ~m_mask;
            setModified(true);
            // Now have to recurse to do any further bypassing that may be required
            // E.g. bypass the two recursive calls in fibo?? FIXME: check!
            auto bp = std::make_unique<CallBypasser>(m_enclosingStmt);
            return ret->acceptModifier(bp.get());
        }
    }

    // Else just leave as is (perhaps simplified)
    return ret;
}


SharedExp CallBypasser::postModify(const std::shared_ptr<Location> &exp)
{
    return exp->simplify();
}

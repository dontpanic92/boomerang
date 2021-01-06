#pragma region License
/*
 * This file is part of the Boomerang Decompiler.
 *
 * See the file "LICENSE.TERMS" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 */
#pragma endregion License


#include "boomerang/visitor/stmtmodifier/StmtModifier.h"


class ExpSubscripter;


class BOOMERANG_API StmtSubscripter : public StmtModifier
{
public:
    StmtSubscripter(ExpSubscripter *es);
    ~StmtSubscripter() override = default;

public:
    /// \copydoc StmtModifier::visit
    void visit(const std::shared_ptr<Assign> &stmt, bool &visitChildren) override;

    /// \copydoc StmtModifier::visit
    void visit(const std::shared_ptr<PhiAssign> &stmt, bool &visitChildren) override;

    /// \copydoc StmtModifier::visit
    void visit(const std::shared_ptr<ImplicitAssign> &stmt, bool &visitChildren) override;

    /// \copydoc StmtModifier::visit
    void visit(const std::shared_ptr<BoolAssign> &stmt, bool &visitChildren) override;

    /// \copydoc StmtModifier::visit
    void visit(const std::shared_ptr<CallStatement> &stmt, bool &visitChildren) override;
};

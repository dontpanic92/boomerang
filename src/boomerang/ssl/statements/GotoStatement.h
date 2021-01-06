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


#include "boomerang/ssl/statements/Statement.h"


/**
 * GotoStatement has just one member variable, an expression representing the jump's destination
 * (an integer constant for direct jumps; an expression for register jumps).
 * An instance of this class will never represent a return or computed call as these are
 * distinguished by the frontend and are instantiated as CallStatements and ReturnStatements
 * respecitvely.
 * This class also represents unconditional jumps with a fixed offset.
 */
class BOOMERANG_API GotoStatement : public Statement
{
public:
    /// Construct a jump to a fixed address \p jumpDest
    GotoStatement(Address jumpDest);
    GotoStatement(SharedExp dest);
    GotoStatement(const GotoStatement &other) = default;
    GotoStatement(GotoStatement &&other)      = default;

    ~GotoStatement() override;

    GotoStatement &operator=(const GotoStatement &other) = default;
    GotoStatement &operator=(GotoStatement &&other) = default;

public:
    /// \copydoc Statement::clone
    SharedStmt clone() const override;

    /// \copydoc Statement::accept
    bool accept(StmtVisitor *visitor) const override;

    /// \copydoc Statement::accept
    bool accept(StmtExpVisitor *visitor) override;

    /// \copydoc Statement::accept
    bool accept(StmtModifier *modifier) override;

    /// \copydoc Statement::accept
    bool accept(StmtPartModifier *modifier) override;

    /// Set the destination of this jump to be a given fixed address.
    void setDest(Address addr);

    /// Set the destination of this jump to be an expression (computed jump)
    void setDest(SharedExp pd);

    /// Returns the destination of this CTI.
    virtual SharedExp getDest();
    virtual const SharedExp getDest() const;

    /// \returns Fixed destination address or Address::INVALID if there isn't one.
    /// For dynamic CTIs, returns Address::INVALID.
    Address getFixedDest() const;

    /**
     * Sets the fact that this call is computed.
     * \todo This should really be removed, once CaseStatement
     * and HLNwayCall are implemented properly
     */
    void setIsComputed(bool computed = true);

    /**
     * Returns whether or not this call is computed.
     * \todo This should really be removed, once CaseStatement
     * and HLNwayCall are implemented properly
     * \returns this call is computed
     */
    bool isComputed() const;

    /// \copydoc Statement::print
    void print(OStream &os) const override;

    /// \copydoc Statement::search
    bool search(const Exp &pattern, SharedExp &result) const override;

    /// \copydoc Statement::searchAndReplace
    bool searchAll(const Exp &search, std::list<SharedExp> &result) const override;

    /// \copydoc Statement::searchAndReplace
    bool searchAndReplace(const Exp &pattern, SharedExp replace, bool cc = false) override;

    // simplify all the uses/defs in this Statement
    void simplify() override;

protected:
    /// Destination of a jump or call. This is the absolute destination
    /// for both static and dynamic CTIs.
    SharedExp m_dest;
    bool m_isComputed; ///< True if this is a CTI with a computed destination address.
};

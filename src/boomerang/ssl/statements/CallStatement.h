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


#include "boomerang/db/DefCollector.h"
#include "boomerang/db/UseCollector.h"
#include "boomerang/ssl/statements/Assignment.h"
#include "boomerang/ssl/statements/GotoStatement.h"
#include "boomerang/util/StatementList.h"


class ImplicitAssign;
class ReturnStatement;
class Signature;
class Prog;


/**
 * Represents a high level call.
 * Information about parameters and the like is stored here.
 */
class BOOMERANG_API CallStatement : public GotoStatement
{
public:
    CallStatement(Address dest);
    CallStatement(SharedExp dest);
    CallStatement(const CallStatement &other);
    CallStatement(CallStatement &&other) = default;

    ~CallStatement() override;

    CallStatement &operator=(const CallStatement &other);
    CallStatement &operator=(CallStatement &&other) = default;

public:
    /// \copydoc GotoStatement::clone
    SharedStmt clone() const override;

    /// \copydoc GotoStatement::accept
    bool accept(StmtVisitor *visitor) const override;

    /// \copydoc GotoStatement::accept
    bool accept(StmtExpVisitor *visitor) override;

    /// \copydoc GotoStatement::accept
    bool accept(StmtModifier *modifier) override;

    /// \copydoc GotoStatement::accept
    bool accept(StmtPartModifier *modifier) override;

    /// \copydoc Statement::setNumber
    void setNumber(int num) override;

    /// \copydoc Statement::getDefinitions
    void getDefinitions(LocationSet &defs, bool assumeABICompliance) const override;

    /// \copydoc Statement::definesLoc
    bool definesLoc(SharedExp loc) const override; // True if this Statement defines loc

    /// \copydoc GotoStatement::search
    bool search(const Exp &search, SharedExp &result) const override;

    /// \copydoc GotoStatement::searchAll
    bool searchAll(const Exp &search, std::list<SharedExp> &result) const override;

    /// \copydoc GotoStatement::searchAndReplace
    bool searchAndReplace(const Exp &search, SharedExp replace, bool cc = false) override;

    /// \copydoc GotoStatement::simplify
    void simplify() override;

    /// \copydoc Statement::getTypeForExp
    SharedConstType getTypeForExp(SharedConstExp exp) const override;

    /// \copydoc Statement::getTypeForExp
    SharedType getTypeForExp(SharedExp exp) override;

    /// \copydoc Statement::setTypeForExp
    void setTypeForExp(SharedExp exp, SharedType ty) override;

    /// \copydoc GotoStatement::print
    void print(OStream &os) const override;

public:
    /// Return call's arguments
    StatementList &getArguments() { return m_arguments; }
    const StatementList &getArguments() const { return m_arguments; }

    /// Set the arguments of this call. Takes ownership of the statements
    /// in \p args.
    /// \param args The list of locations to set the arguments to (for testing)
    void setArguments(const StatementList &args);

    /// Set the arguments of this call based on signature info
    /// \note Should only be called for calls to library functions
    void setSigArguments();

    /// Update the arguments based on a callee change
    void updateArguments();

    SharedExp getArgumentExp(int i) const;

    void setArgumentExp(int i, SharedExp e);

    int getNumArguments() const;

    void setNumArguments(int i);

    void removeArgument(int i);

    SharedType getArgumentType(int i) const;

    void setArgumentType(int i, SharedType ty);

    void eliminateDuplicateArgs();

public:
    /// Set the function that is called by this call statement.
    void setDestProc(Function *dest);

    /// \returns the function that is called by this call statement.
    Function *getDestProc();
    const Function *getDestProc() const;

    /**
     * Sets a bit that says that this call is effectively followed by a return.
     * This happens e.g. on x86 for tail call jumps (i.e jmp instead of call/ret)
     * \param b true if this is to be set; false to clear the bit
     */
    void setReturnAfterCall(bool b);

    /// \returns True if this call is effectively followed by a return
    bool isReturnAfterCall() const;

    bool isChildless() const;

    bool isCallToMemOffset() const;

    std::shared_ptr<Signature> getSignature() { return m_signature; }
    void setSignature(const std::shared_ptr<Signature> &sig) { m_signature = sig; }

public:
    /// \returns pointer to the def collector object
    const DefCollector *getDefCollector() const { return &m_defCol; }
    DefCollector *getDefCollector() { return &m_defCol; }

    /// \returns pointer to the use collector object
    const UseCollector *getUseCollector() const { return &m_useCol; }
    UseCollector *getUseCollector() { return &m_useCol; }

    /// Add x to the UseCollector for this call
    void useBeforeDefine(SharedExp x) { m_useCol.collectUse(x); }

    /// Remove e from the UseCollector
    void removeLiveness(SharedExp e) { m_useCol.removeUse(e); }

    /// Remove all livenesses
    void removeAllLive() { m_useCol.clear(); }

    /// direct call
    void useColfromSSAForm(const SharedStmt &s) { m_useCol.fromSSAForm(m_proc, s); }

public:
    /// For testing.
    void addDefine(const std::shared_ptr<ImplicitAssign> &as);

    /// Temporarily needed for ad-hoc type analysis
    /// \returns true if removed successfully
    bool removeDefine(SharedExp e);

    /// Get list of locations defined by this call
    const StatementList &getDefines() const { return m_defines; }
    StatementList &getDefines() { return m_defines; }

    void setDefines(const StatementList &defines);

    /// Find the reaching definition for expression e.
    /// Find the definition for the given expression, using the embedded Collector object
    /// Was called findArgument(), and used implicit arguments and signature parameters
    /// \note must only operator on unsubscripted locations, otherwise it is invalid
    SharedExp findDefFor(SharedExp e) const;

public:
    /// Calculate results(this) = defines(this) intersect live(this)
    /// \note could use a LocationList for this, but then there is nowhere to store the types
    /// (for DFA based TA). So the RHS is just ignored
    std::unique_ptr<StatementList> calcResults() const;

    /// \returns the one and only return statement of the called proc
    const std::shared_ptr<ReturnStatement> &getCalleeReturn() { return m_calleeReturn; }
    void setCalleeReturn(const std::shared_ptr<ReturnStatement> &ret) { m_calleeReturn = ret; }

    SharedExp getProven(SharedExp e);

    /// Localise the various components of expression e with reaching definitions to this call
    /// Note: can change e so usually need to clone the argument
    /// Was called substituteParams
    ///
    /// Substitute the various components of expression e with the appropriate reaching definitions.
    /// Used in e.g. fixCallBypass (via the CallBypasser). Locations defined in this call are
    /// replaced with their proven values, which are in terms of the initial values at the start of
    /// the call (reaching definitions at the call)
    SharedExp localiseExp(SharedExp e);

    /// Localise only components of e, i.e. xxx if e is m[xxx]
    void localiseComp(SharedExp e);

    // Do the call bypass logic e.g. r28{20} -> r28{17} + 4 (where 20 is this CallStatement)
    // Set ch if changed (bypassed)
    SharedExp bypassRef(const std::shared_ptr<RefExp> &r, bool &changed);

    /// Process this call for ellipsis parameters. If found, in a printf/scanf call, truncate the
    /// number of parameters if needed, and return true if any signature parameters added. This
    /// function has two jobs. One is to truncate the list of arguments based on the format string.
    /// The second is to add parameter types to the signature.
    /// If -Td is used, type analysis will be rerun with these changes.
    bool doEllipsisProcessing();

    /// Attempt to convert this call, if indirect, to a direct call.
    /// NOTE: at present, we igore the possibility that some other statement
    /// will modify the global. This is a serious limitation!!
    /// \returns true if converted
    bool tryConvertToDirect();

private:
    bool doObjCEllipsisProcessing(const QString &formatStr);

    /// Parses the format string and extracts all signature types.
    /// \returns the number of new types added, or -1 on failure.
    int parseFmtStr(const QString &fmtStr, bool isScanf);

    /// Private helper functions for the above
    /// If addPointer is true, add type 'ty *' to the signature instead of type 'ty'
    void addSigParam(SharedType ty, bool addPointer);

    /// Make an assign suitable for use as an argument from a callee context expression
    std::shared_ptr<Assign> makeArgAssign(SharedType ty, SharedExp e);

private:
    bool m_returnAfterCall = false; // True if call is effectively followed by a return.

    /// The list of arguments passed by this call, actually a list of Assign statements (location :=
    /// expr)
    StatementList m_arguments;

    /// The list of defines for this call, a list of ImplicitAssigns (used to be called returns).
    /// Essentially a localised copy of the modifies of the callee, so the callee could be deleted.
    /// Stores types and locations.  Note that not necessarily all of the defines end up being
    /// declared as results.
    StatementList m_defines;

    /// Destination of call. In the case of an analysed indirect call,
    /// this will be ONE target's return statement. For an unanalysed indirect call,
    /// or a call whose callee is not yet sufficiently decompiled due to recursion,
    /// this will be nullptr
    Function *m_procDest = nullptr;

    /// The signature for this call.
    /// \note this used to be stored in the Proc, but this does not make sense
    /// when the proc happens to have varargs
    std::shared_ptr<Signature> m_signature;

    /// A UseCollector object to collect the live variables at this call.
    /// Used as part of the calculation of results
    UseCollector m_useCol;

    /// A DefCollector object to collect the reaching definitions;
    /// used for bypassAndPropagate/localiseExp etc; also
    /// the basis for arguments if this is an unanlysed indirect call
    DefCollector m_defCol;

    /// Pointer to the callee ReturnStatement. If the callee is unanlysed,
    /// this will be a special ReturnStatement with ImplicitAssigns.
    /// Callee could be unanalysed because of an unanalysed indirect call,
    /// or a "recursion break".
    std::shared_ptr<ReturnStatement> m_calleeReturn = nullptr;
};

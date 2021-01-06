#pragma region License
/*
 * This file is part of the Boomerang Decompiler.
 *
 * See the file "LICENSE.TERMS" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 */
#pragma endregion License
#include "DataFlow.h"

#include "boomerang/core/Project.h"
#include "boomerang/core/Settings.h"
#include "boomerang/db/IRFragment.h"
#include "boomerang/db/Prog.h"
#include "boomerang/db/proc/ProcCFG.h"
#include "boomerang/db/proc/UserProc.h"
#include "boomerang/ssl/exp/RefExp.h"
#include "boomerang/ssl/exp/Terminal.h"
#include "boomerang/ssl/statements/Assign.h"
#include "boomerang/ssl/statements/CallStatement.h"
#include "boomerang/ssl/statements/PhiAssign.h"
#include "boomerang/util/log/Log.h"
#include "boomerang/visitor/expmodifier/ExpSSAXformer.h"
#include "boomerang/visitor/expmodifier/ImplicitConverter.h"

#include <cstring>
#include <sstream>


DataFlow::DataFlow(UserProc *proc)
    : m_proc(proc)
    , renameLocalsAndParams(false)
{
}


DataFlow::~DataFlow()
{
}


void DataFlow::dfs(FragIndex myIdx, FragIndex parentIdx)
{
    assert(myIdx != INDEX_INVALID);
    if (m_dfnum[myIdx] >= 0) {
        // already visited
        return;
    }

    m_dfnum[myIdx]  = N;
    m_vertex[N]     = myIdx;
    m_parent[myIdx] = parentIdx;

    N++;

    // Recurse to successors
    IRFragment *frag = m_frags[myIdx];

    for (IRFragment *succ : frag->getSuccessors()) {
        dfs(m_indices[succ], myIdx);
    }
}


bool DataFlow::calculateDominators()
{
    ProcCFG *cfg               = m_proc->getCFG();
    IRFragment *entryFrag      = cfg->getEntryFragment();
    const std::size_t numFrags = cfg->getNumFragments();

    if (!entryFrag || numFrags == 0) {
        return false; // nothing to do
    }

    allocateData();
    recalcSpanningTree();

    assert(N >= 1);

    // Process fragments in reverse pre-traversal order (i.e. return blocks first)
    for (std::size_t i = N - 1; i >= 1; i--) {
        FragIndex n = m_vertex[i];
        FragIndex p = m_parent[n];
        FragIndex s = p;

        // These lines calculate the semi-dominator of n, based on the Semidominator Theorem
        for (IRFragment *pred : m_frags[n]->getPredecessors()) {
            if (m_indices.find(pred) == m_indices.end()) {
                LOG_ERROR("Fragment not in indices: ", pred->toString());
                return false;
            }

            const FragIndex v = m_indices[pred];
            FragIndex sdash   = v;

            if (isAncestorOf(v, n)) {
                sdash = m_semi[getAncestorWithLowestSemi(v)];
            }

            if (isAncestorOf(s, sdash)) {
                s = sdash;
            }
        }

        m_semi[n] = s;

        // Calculation of n's dominator is deferred until the path from s to n
        // has been linked into the forest
        m_bucket[s].insert(n);
        link(p, n);

        // for each v in bucket[p]
        for (FragIndex v : m_bucket[p]) {
            // Now that the path from p to v has been linked into the spanning forest,
            // these lines calculate the dominator of v, based on the first clause of the
            // Dominator Theorem, or else defer the calculation until y's dominator is known.
            const FragIndex y = getAncestorWithLowestSemi(v);

            if (m_semi[y] == m_semi[v]) {
                m_idom[v] = p; // Success!
            }
            else {
                m_samedom[v] = y; // Defer
            }
        }

        m_bucket[p].clear();
    }

    for (std::size_t i = 1; i < N - 1; i++) {
        // Now all the deferred dominator calculations, based on the second clause of the Dominator
        // Theorem, are performed.
        FragIndex n = m_vertex[i];

        if (m_samedom[n] != INDEX_INVALID) {
            m_idom[n] = m_idom[m_samedom[n]]; // Deferred success!
        }
    }

    const FragIndex entryIndex = fragToIdx(entryFrag);
    assert(entryIndex != INDEX_INVALID);

    // the entry fragment is always executed.
    m_idom[entryIndex] = entryIndex;
    m_semi[entryIndex] = entryIndex;

    computeDF(entryIndex); // Finally, compute the dominance frontiers
    return true;
}


FragIndex DataFlow::getAncestorWithLowestSemi(FragIndex v)
{
    assert(v != INDEX_INVALID);

    const FragIndex a = m_ancestor[v];
    if (a != INDEX_INVALID && m_ancestor[a] != INDEX_INVALID) {
        const FragIndex b = getAncestorWithLowestSemi(a);
        m_ancestor[v]     = m_ancestor[a];

        if (isAncestorOf(m_semi[m_best[v]], m_semi[b])) {
            m_best[v] = b;
        }
    }

    return m_best[v];
}


void DataFlow::link(FragIndex p, FragIndex n)
{
    assert(n != INDEX_INVALID);

    m_ancestor[n] = p;
    m_best[n]     = n;
}


bool DataFlow::doesDominate(FragIndex n, FragIndex w)
{
    assert(n != INDEX_INVALID);
    assert(w != INDEX_INVALID);

    while (w != INDEX_INVALID && m_idom[w] != w) {
        if (m_idom[w] == n) {
            return true;
        }

        w = m_idom[w]; // Move up the dominator tree
    }

    return false;
}


void DataFlow::computeDF(FragIndex n)
{
    assert(n != INDEX_INVALID);

    std::set<FragIndex> S;
    // This loop computes DF_local[n]
    // for each node y in succ(n)
    IRFragment *frag = m_frags[n];

    for (IRFragment *succ : frag->getSuccessors()) {
        FragIndex y = m_indices[succ];

        if (m_idom[y] != n) {
            S.insert(y);
        }
    }

    // for each child c of n in the dominator tree
    // Note: this is a linear search!
    const size_t sz = m_idom.size(); // ? Was ancestor.size()

    for (FragIndex c = FragIndex(0); c < sz; ++c) {
        if (m_idom[c] != n) {
            continue;
        }
        else if (c != n) { // do not calculate DF for entry fragment again
            computeDF(c);
        }

        /* This loop computes DF_up[c] */
        // for each element w of DF[c]
        std::set<FragIndex> &s = m_DF[c];

        for (FragIndex w : s) {
            if (n == w || !doesDominate(n, w)) {
                S.insert(w);
            }
        }
    }

    m_DF[n] = S;
}


bool DataFlow::canRename(SharedConstExp exp) const
{
    if (exp->isSubscript()) {
        exp = exp->getSubExp1(); // Look inside refs
    }

    if (exp->isRegOf() ||    // Always rename registers
        exp->isTemp() ||     // Always rename temps (always want to propagate away)
        exp->isFlags() ||    // Always rename flags
        exp->isMainFlag() || // Always rename individual flags like %CF
        exp->isLocal()) {    // Rename hard locals in the post fromSSA pass
        return true;
    }

    if (!exp->isMemOf()) {
        return false; // Can't rename %pc or other junk
    }

    // I used to check here if there was a symbol for the memory expression, and if so allow it to
    // be renamed. However, even named locals and parameters could have their addresses escape the
    // local function, so we need another test anyway. So locals and parameters should not be
    // renamed (and hence propagated) until escape analysis is done (and hence renaleLocalsAndParams
    // is set) Besides,  before we have types and references, it is not easy to find a type for the
    // location, so we can't tell if e.g. m[esp{-}+12] is evnp or a separate local. It certainly
    // needs to have the local/parameter pattern
    if (!m_proc->isLocalOrParamPattern(exp)) {
        return false;
    }

    // e is a local or parameter; allow it to be propagated iff we've done escape analysis and the
    // address has not
    return renameLocalsAndParams;
}


bool DataFlow::placePhiFunctions()
{
    // First free some memory no longer needed
    m_dfnum.resize(0);
    m_semi.resize(0);
    m_ancestor.resize(0);
    m_samedom.resize(0);
    m_vertex.resize(0);
    m_parent.resize(0);
    m_best.resize(0);
    m_bucket.resize(0);
    m_defsites.clear();
    m_defallsites.clear();

    for (ExSet &exps : m_definedAt) {
        for (auto iter = exps.begin(); iter != exps.end();) {
            if (m_A_phi.find(*iter) == m_A_phi.end()) {
                iter = exps.erase(iter);
            }
            else {
                ++iter;
            }
        }
    }

    m_definedAt.clear(); // and A_orig,
    m_defStmts.clear();  // and the map from variable to defining Stmt

    for (IRFragment *frag : *m_proc->getCFG()) {
        frag->clearPhis();
    }

    // Set the sizes of needed vectors
    const std::size_t numIndices = m_indices.size();
    const std::size_t numFrags   = m_proc->getCFG()->getNumFragments();
    assert(numIndices == numFrags);
    Q_UNUSED(numIndices);

    m_definedAt.resize(numFrags);

    const bool assumeABICompliance = m_proc->getProg()->getProject()->getSettings()->assumeABI;

    // We need to create m_definedAt[n] for all n
    // Recreate each call because propagation and other changes make old data invalid
    for (FragIndex n{ 0 }; n < numFrags; ++n) {
        IRFragment::RTLIterator rit;
        StatementList::iterator sit;
        IRFragment *frag = m_frags[n];

        for (SharedStmt stmt = frag->getFirstStmt(rit, sit); stmt;
             stmt            = frag->getNextStmt(rit, sit)) {
            LocationSet locationSet;
            stmt->getDefinitions(locationSet, assumeABICompliance);

            // If this is a childless call, then this block defines every variable
            if (stmt->isCall() && stmt->as<CallStatement>()->isChildless()) {
                m_defallsites.insert(n);
            }

            for (const SharedExp &exp : locationSet) {
                if (canRename(exp)) {
                    m_definedAt[n].insert(exp->clone());
                    m_defStmts[exp] = stmt;
                }
            }
        }
    }

    for (FragIndex n{ 0 }; n < numFrags; ++n) {
        for (const SharedExp &a : m_definedAt[n]) {
            m_defsites[a].insert(n);
        }
    }

    bool change = false;

    // Those variables that are defined everywhere (i.e. in defallsites)
    // need to be defined at every defsite, too
    for (FragIndex defallsite : m_defallsites) {
        for (auto &[exp, defsites] : m_defsites) {
            Q_UNUSED(exp);
            defsites.insert(defallsite);
        }
    }

    // For each variable a defined anywhere
    for (auto &[a, defsites] : m_defsites) {
        std::set<FragIndex> W = defsites;

        while (!W.empty()) {
            // Pop first node from W
            const FragIndex n = *W.begin();
            W.erase(W.begin());

            for (FragIndex y : m_DF[n]) {
                // phi function already created for y?
                if (m_A_phi[a].find(y) != m_A_phi[a].end()) {
                    continue;
                }

                // Insert trivial phi function for a at top of block y: a := phi()
                change = true;
                m_frags[y]->addPhi(a->clone());

                // A_phi[a] <- A_phi[a] U {y}
                m_A_phi[a].insert(y);

                // if a !elementof A_orig[y]
                if (!m_definedAt[y].contains(a)) {
                    // W <- W U {y}
                    W.insert(y);
                }
            }
        }
    }

    return change;
}


void DataFlow::convertImplicits()
{
    ProcCFG *cfg = m_proc->getCFG();

    // Convert statements in A_phi from m[...]{-} to m[...]{0}
    std::map<SharedExp, std::set<FragIndex>, lessExpStar> A_phi_copy = m_A_phi; // Object copy
    ImplicitConverter ic(cfg);
    m_A_phi.clear();

    for (auto &[exp, set] : A_phi_copy) {
        SharedExp e = exp->clone()->acceptModifier(&ic);
        m_A_phi[e]  = set; // Copy the set (doesn't have to be deep)
    }

    std::map<SharedExp, std::set<FragIndex>, lessExpStar> defsites_copy = m_defsites; // Object copy
    m_defsites.clear();

    for (auto &[exp, set] : defsites_copy) {
        SharedExp e   = exp->clone()->acceptModifier(&ic);
        m_defsites[e] = set; // Copy the set (doesn't have to be deep)
    }

    std::vector<ExSet> definedAtCopy = m_definedAt;
    m_definedAt.clear();

    for (ExSet &se : definedAtCopy) {
        ExSet se_new;

        for (const SharedExp &ee : se) {
            SharedExp e = ee->clone()->acceptModifier(&ic);
            se_new.insert(e);
        }

        // Copy the set (doesn't have to be a deep copy)
        m_definedAt.insert(m_definedAt.end(), se_new);
    }
}


void DataFlow::allocateData()
{
    ProcCFG *cfg               = m_proc->getCFG();
    const std::size_t numFrags = cfg->getNumFragments();

    m_frags.assign(numFrags, nullptr);
    m_indices.clear();

    m_dfnum.assign(numFrags, -1);
    m_semi.assign(numFrags, INDEX_INVALID);
    m_ancestor.assign(numFrags, INDEX_INVALID);
    m_idom.assign(numFrags, INDEX_INVALID);
    m_samedom.assign(numFrags, INDEX_INVALID);
    m_vertex.assign(numFrags, INDEX_INVALID);
    m_parent.assign(numFrags, INDEX_INVALID);
    m_best.assign(numFrags, INDEX_INVALID);
    m_bucket.assign(numFrags, {});
    m_DF.assign(numFrags, {});
    m_definedAt.assign(numFrags, {});

    m_A_phi.clear();
    m_defsites.clear();
    m_defallsites.clear();
    m_defStmts.clear();

    // Set up the fragment and indices vectors.
    // Do this here because sometimes a fragment can be unreachable
    // (so relying on in-edges doesn't work)
    std::size_t i = 0;
    for (IRFragment *frag : *cfg) {
        m_frags[i++] = frag;
    }

    for (std::size_t j = 0; j < numFrags; j++) {
        m_indices[m_frags[j]] = j;
    }
}


void DataFlow::recalcSpanningTree()
{
    IRFragment *entryFrag = m_proc->getEntryFragment();
    assert(entryFrag != nullptr);
    const FragIndex entryIndex = fragToIdx(entryFrag);
    assert(entryIndex != INDEX_INVALID);

    N = 0;
    dfs(entryIndex, INDEX_INVALID);
}


bool DataFlow::isAncestorOf(FragIndex n, FragIndex parent) const
{
    return m_dfnum[parent] < m_dfnum[n];
}

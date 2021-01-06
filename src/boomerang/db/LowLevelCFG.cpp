#pragma region License
/*
 * This file is part of the Boomerang Decompiler.
 *
 * See the file "LICENSE.TERMS" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 */
#pragma endregion License
#include "LowLevelCFG.h"

#include "boomerang/db/BasicBlock.h"
#include "boomerang/db/proc/UserProc.h"
#include "boomerang/util/log/Log.h"

#include <numeric>


LowLevelCFG::LowLevelCFG()
{
}


LowLevelCFG::~LowLevelCFG()
{
    for (BasicBlock *bb : *this) {
        delete bb;
    }
}


int LowLevelCFG::getNumBBs() const
{
    return m_bbStartMap.size();
}


BasicBlock *LowLevelCFG::createBB(BBType bbType, const std::vector<MachineInstruction> &bbInsns)
{
    assert(!bbInsns.empty());

    // First find the native address of the first instruction
    const Address startAddr = bbInsns.front().m_addr;
    assert(startAddr != Address::INVALID);

    BasicBlock *currentBB   = nullptr;
    BBStartMap::iterator mi = m_bbStartMap.find(startAddr);

    if ((mi != m_bbStartMap.end()) && mi->second) {
        currentBB = mi->second;

        // It should be incomplete, or the BB there should be zero
        // (we have called ensureBBExists() but not yet created the BB for it).
        // Else we have duplicated BBs.
        // Note: this can happen with forward jumps into the middle of a loop,
        // so not error
        if (currentBB->isComplete()) {
            LOG_VERBOSE("Not creating a BB at address %1 because a BB already exists",
                        currentBB->getLowAddr());
            return nullptr;
        }
        else {
            // Fill in the details, and return it
            currentBB->completeBB(bbInsns);
            currentBB->setType(bbType);
        }
    }

    if (currentBB == nullptr) {
        currentBB = new BasicBlock(bbType, bbInsns);

        // Note that currentBB->getLowAddr() == startAddr
        if (startAddr == Address::INVALID) {
            LOG_FATAL("Cannot add BB with invalid lowAddr %1", startAddr);
        }

        insertBB(currentBB);
        mi = m_bbStartMap.find(startAddr);
    }

    if (mi != m_bbStartMap.end()) {
        //
        //  Existing   New         +---+ "low" part of new
        //            +---+        +---+
        //            |   |          |   Fall through
        //    +---+   |   |   ==>  +---+
        //    |   |   |   |        |   | Existing; rest of new discarded
        //    +---+   +---+        +---+
        //
        // Check for overlap of the just added BB with the next BB (address wise).
        // If there is an overlap, truncate the RTLList for the new BB to not overlap,
        // and make this a fall through BB.
        // We still want to do this even if the new BB overlaps with an incomplete BB,
        // though in this case, splitBB needs to fill in the details for the "high"
        // BB of the split.
        // Also, in this case, we return a pointer to the newly completed BB,
        // so it will get out edges added (if required). In the other case
        // (i.e. we overlap with an existing, completed BB), we want to return 0, since
        // the out edges are already created.
        //
        mi = std::next(mi);

        if (mi != m_bbStartMap.end()) {
            BasicBlock *nextBB    = (*mi).second;
            Address nextAddr      = (*mi).first;
            bool nextIsIncomplete = !nextBB->isComplete();

            if (nextAddr < currentBB->getHiAddr()) {
                // Need to truncate the current BB. We use splitBB(), but pass it nextBB so it
                // doesn't create a new BB for the "bottom" BB of the split pair
                splitBB(currentBB, nextAddr, nextBB);

                // If the overlapped BB was incomplete, return the "bottom" part of the BB, so
                // adding out edges will work properly.
                if (nextIsIncomplete) {
                    assert(nextBB);
                    return nextBB;
                }

                LOG_VERBOSE("Not creating a BB at address %1 because a BB already exists",
                            currentBB->getLowAddr());
                return nullptr;
            }
        }

        //  Existing    New        +---+ Top of existing
        //    +---+                +---+
        //    |   |    +---+       +---+ Fall through
        //    |   |    |   | =>    |   |
        //    |   |    |   |       |   | New; rest of existing discarded
        //    +---+    +---+       +---+
        //
        // Note: no need to check the other way around, because in this case,
        // we will have called ensureBBExists(), which will have split
        // the existing BB already.
    }

    assert(currentBB);
    return currentBB;
}


BasicBlock *LowLevelCFG::createBB(BBType bbType, const std::list<MachineInstruction> &insns)
{
    std::vector<MachineInstruction> bbInsns(insns.begin(), insns.end());
    return createBB(bbType, bbInsns);
}


BasicBlock *LowLevelCFG::createIncompleteBB(Address lowAddr)
{
    BasicBlock *newBB = new BasicBlock(lowAddr);
    insertBB(newBB);
    return newBB;
}


bool LowLevelCFG::ensureBBExists(Address addr, BasicBlock *&currBB)
{
    // check for overlapping incomplete or complete BBs.
    BBStartMap::iterator itExistingBB = m_bbStartMap.lower_bound(addr);

    BasicBlock *overlappingBB = nullptr;
    if (itExistingBB != m_bbStartMap.end() && itExistingBB->second->getLowAddr() == addr) {
        overlappingBB = itExistingBB->second;
    }
    else if (itExistingBB != m_bbStartMap.begin()) {
        --itExistingBB;
        if (itExistingBB->second && itExistingBB->second->getLowAddr() <= addr &&
            itExistingBB->second->getHiAddr() > addr) {
            overlappingBB = itExistingBB->second;
        }
    }

    if (!overlappingBB) {
        // no BB at addr -> create a new incomplete BB
        createIncompleteBB(addr);
        return false;
    }
    else if (!overlappingBB->isComplete()) {
        return false;
    }
    else if (overlappingBB && overlappingBB->getLowAddr() < addr) {
        splitBB(overlappingBB, addr);
        BasicBlock *highBB = getBBStartingAt(addr);

        if (currBB == overlappingBB) {
            // This means that the BB that we are expecting to use, usually to add
            // out edges, has changed. We must change this pointer so that the right
            // BB gets the out edges. However, if the new BB is not the BB of
            // interest, we mustn't change currBB
            currBB = highBB;
        }
        return true;
    }
    else {
        // addr is the start of a complete BB
        return true;
    }
}


bool LowLevelCFG::isStartOfBB(Address addr) const
{
    return getBBStartingAt(addr) != nullptr;
}


bool LowLevelCFG::isStartOfCompleteBB(Address addr) const
{
    const BasicBlock *bb = getBBStartingAt(addr);
    return bb && bb->isComplete();
}


void LowLevelCFG::removeBB(BasicBlock *bb)
{
    if (bb == nullptr) {
        return;
    }

    BBStartMap::iterator firstIt, lastIt;
    std::tie(firstIt, lastIt) = m_bbStartMap.equal_range(bb->getLowAddr());

    for (auto it = firstIt; it != lastIt; ++it) {
        if (it->second == bb) {
            m_bbStartMap.erase(it);
            delete bb;
            return;
        }
    }

    LOG_WARN("Tried to remove BB at address %1; does not exist in CFG", bb->getLowAddr());
    delete bb;
}


void LowLevelCFG::addEdge(BasicBlock *sourceBB, BasicBlock *destBB)
{
    if (!sourceBB || !destBB) {
        return;
    }

    // Wire up edges
    sourceBB->addSuccessor(destBB);
    destBB->addPredecessor(sourceBB);

    // special handling for upgrading oneway BBs to twoway BBs
    if ((sourceBB->getType() == BBType::Oneway) && (sourceBB->getNumSuccessors() > 1)) {
        sourceBB->setType(BBType::Twoway);
    }
}


void LowLevelCFG::addEdge(BasicBlock *sourceBB, Address addr)
{
    // If we already have a BB for this address, add the edge to it.
    // If not, create a new incomplete BB at the destination address.
    BasicBlock *destBB = getBBStartingAt(addr);

    if (!destBB) {
        destBB = createIncompleteBB(addr);
    }

    this->addEdge(sourceBB, destBB);
}


bool LowLevelCFG::isWellFormed() const
{
    for (const BasicBlock *bb : *this) {
        if (!bb->isComplete()) {
            LOG_ERROR("CFG is not well formed: BB at address %1 is incomplete", bb->getLowAddr());
            return false;
        }

        for (const BasicBlock *pred : bb->getPredecessors()) {
            if (!pred->isPredecessorOf(bb)) {
                LOG_ERROR("CFG is not well formed: Edge from BB at %1 to BB at %2 is malformed.",
                          pred->getLowAddr(), bb->getLowAddr());
                return false;
            }
            else if (pred->getProc() != bb->getProc()) {
                const UserProc *myFunc   = bb->getProc();
                const UserProc *predFunc = pred->getProc();
                LOG_ERROR("CFG is not well formed: Interprocedural edge from '%1' to '%2' found",
                          myFunc ? myFunc->getName() : "<invalid>",
                          predFunc ? predFunc->getName() : "<invalid>");
                return false;
            }
        }

        for (const BasicBlock *succ : bb->getSuccessors()) {
            if (!succ->isSuccessorOf(bb)) {
                LOG_ERROR("CFG is not well formed: Edge from BB at %1 to BB at %2 is malformed.",
                          bb->getLowAddr(), succ->getLowAddr());
                return false;
            }
            else if (succ->getProc() != bb->getProc()) {
                const UserProc *myFunc   = bb->getProc();
                const UserProc *succFunc = succ->getProc();
                LOG_ERROR("CFG is not well formed: Interprocedural edge from '%1' to '%2' found",
                          myFunc ? myFunc->getName() : "<invalid>",
                          succFunc ? succFunc->getName() : "<invalid>");
                return false;
            }
        }
    }

    return true;
}


BasicBlock *LowLevelCFG::findRetNode()
{
    for (BasicBlock *bb : *this) {
        if (bb->getType() == BBType::Ret) {
            return bb;
        }
    }

    return nullptr;
}


BasicBlock *LowLevelCFG::splitBB(BasicBlock *bb, Address splitAddr, BasicBlock *_newBB /* = 0 */)
{
    std::vector<MachineInstruction>::iterator splitIt;

    // First find which RTL has the split address; note that this could fail
    // (e.g. jump into the middle of an instruction)
    for (splitIt = bb->getInsns().begin(); splitIt != bb->getInsns().end(); ++splitIt) {
        if (splitIt->m_addr == splitAddr) {
            break;
        }
    }

    if (splitIt == bb->getInsns().end()) {
        LOG_WARN("Cannot split BB at address %1 at split address %2", bb->getLowAddr(), splitAddr);
        return bb;
    }

    if (_newBB && _newBB->isComplete()) {
        // we already have a BB for the high part. Delete overlapping RTLs and adjust edges.

        while (splitIt != bb->getInsns().end()) {
            splitIt = bb->getInsns().erase(splitIt); // deletes RTLs
        }

        _newBB->removeAllPredecessors();
        for (BasicBlock *succ : bb->getSuccessors()) {
            succ->removePredecessor(bb);
        }

        bb->removeAllSuccessors();
        addEdge(bb, _newBB);
        bb->setType(BBType::Fall);
        insertBB(_newBB);
        return _newBB;
    }
    else if (!_newBB) {
        // create a new incomplete BasicBlock.
        _newBB = createIncompleteBB(splitAddr);
    }

    // Now we have an incomplete BB at splitAddr;
    // just complete it with the "high" RTLs from the original BB.
    // We don't want to "deep copy" the RTLs themselves,
    // because we want to transfer ownership from the original BB to the "high" part
    std::vector<MachineInstruction> highInsns(splitIt, bb->getInsns().end());
    bb->getInsns().erase(splitIt, bb->getInsns().end());

    _newBB->completeBB(highInsns);

    assert(_newBB->getNumPredecessors() == 0);
    assert(_newBB->getNumSuccessors() == 0);

    const std::vector<BasicBlock *> &successors = bb->getSuccessors();
    for (BasicBlock *succ : successors) {
        succ->removePredecessor(bb);
        succ->addPredecessor(_newBB);
        _newBB->addSuccessor(succ);
    }

    bb->removeAllSuccessors();
    addEdge(bb, _newBB);
    _newBB->setType(bb->getType());
    bb->setType(BBType::Fall);
    return _newBB;
}


void LowLevelCFG::print(OStream &out) const
{
    out << "Control Flow Graph:\n";

    for (const BasicBlock *bb : *this) {
        bb->print(out);
    }

    out << '\n';
}


QString LowLevelCFG::toString() const
{
    QString result;
    OStream os(&result);
    print(os);
    return result;
}


void LowLevelCFG::insertBB(BasicBlock *bb)
{
    assert(bb != nullptr);
    assert(bb->getLowAddr() != Address::INVALID);

    m_bbStartMap[bb->getLowAddr()] = bb;
}

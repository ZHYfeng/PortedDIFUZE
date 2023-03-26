//
// Created by machiry on 12/3/16.
//

#ifndef PROJECT_CFGUTILS_H
#define PROJECT_CFGUTILS_H

#include "../ToolLib/basic.h"

using namespace llvm;
namespace IOCTL_CHECKER {
    /***
     *
     */
    class BBTraversalHelper {
    public:
        /***
         * Get the Strongly connected components(SCC) of the CFG of the provided function in topological order
         * @param currF Function whose SCC visiting order needs to be fetched.
         * @return vector of vector of BasicBlocks.
         *     i.e vector of SCCs
         */
        static std::vector<std::vector<BasicBlock *> *> *getSCCTraversalOrder(Function &currF);

        /***
         * Get number of times all the BBs in the provided strongly connected component need to be analyzed
         * So that all the information is propagated correctly.
         * @param currSCC vector of BBs in the Strongly connected component.
         * @return number of times all the BBs needs to be analyzed to ensure
         * that all the information with in SCC is properly propogated.
         */
        static unsigned long getNumTimesToAnalyze(std::vector<BasicBlock *> *currSCC);

        /***
         *
         * @param A
         * @param B
         * @return
         */
        static bool isPotentiallyReachable(const Instruction *A, const Instruction *B);

        /***
         * Checks whether a path exists from startInstr to endInstr along provided callSites.
         *
         * @param startInstr src or from instruction from where we need to check for path.
         * @param endInstr dst or to instruction to check for path
         * @param callSites pointer to the vector of callsites through which endInstr is reached from startInstr
         * @return true/false depending on whether a path exists or not.
         */
        static bool isReachable(Instruction *startInstr, Instruction *endInstr, std::vector<Instruction*> *callSites);


        /***
         *
         * Get all reachable basic blocks from the provided basic block.
         *
         * @param startBB The src basic block.
         * @param targetBBs All the possible basic blocks which need to be checked for reachability.
         * @param reachableBBs Ouput containing the list of basic blocks which are reachable from the provided startBB.
         */
        static void getAllReachableBBs(BasicBlock *startBB, std::vector<BasicBlock*> &targetBBs,
                                    std::vector<BasicBlock*> &reachableBBs);
    };
}
#endif //PROJECT_CFGUTILS_H

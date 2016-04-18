#ifndef NODESTK_H_
#define NODESTK_H_

#include <nodes/NodeStoreEx.h>
#include <common/Common.h>
#include <common/toolkit/list/StrCpyListIter.h>
#include <common/toolkit/list/UInt8List.h>


extern fhgfs_bool NodesTk_downloadNodes(App* app, Node* sourceNode, NodeType nodeType,
   NodeList* outNodeList, uint16_t* outRootNodeID);
extern fhgfs_bool NodesTk_downloadTargetMappings(App* app, Node* sourceNode,
   UInt16List* outTargetIDs, UInt16List* outNodeIDs);
extern fhgfs_bool NodesTk_downloadMirrorBuddyGroups(App* app, Node* sourceNode, NodeType nodeType,
   UInt16List* outBuddyGroupIDs, UInt16List* outPrimaryTargetIDs,
   UInt16List* outSecondaryTargetIDs);
extern fhgfs_bool NodesTk_downloadTargetStates(App* app, Node* sourceNode, NodeType nodeType,
   UInt16List* outTargetIDs, UInt8List* outReachabilityStates, UInt8List* outConsistencyStates);
extern fhgfs_bool NodesTk_downloadStatesAndBuddyGroups(App* app, Node* sourceNode,
   UInt16List* outBuddyGroupIDs, UInt16List* outPrimaryTargetIDs, UInt16List* outSecondaryTargetIDs,
   UInt16List* outTargetIDs, UInt8List* outTargetReachabilityStates,
   UInt8List* outTargetConsistencyStates);
extern unsigned NodesTk_dropAllConnsByStore(NodeStoreEx* nodes);


#endif /* NODESTK_H_ */

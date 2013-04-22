/* Copyright Jukka Jyl�nki

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License. */

/** @file KdTree.h
	@author Jukka Jyl�nki
	@brief A KD-tree acceleration structure for static geometry. */
#pragma once

#include "Types.h"
#include "myassert.h"

#ifdef MATH_CONTAINERLIB_SUPPORT
#include "Container/MaxHeap.h"
#endif

#include "AABB.h"

MATH_BEGIN_NAMESPACE

enum CardinalAxis
{
	AxisX = 0,
	AxisY,
	AxisZ,
	AxisNone
};

struct KdTreeNode
{
	/// If this is an inner node, specifies along which axis this node is split. Of type CardinalAxis.
	/// If this is a leaf, has the value AxisNone.
	unsigned splitAxis : 2;
	/// If this is an inner node, specifies the index/offset to the child node pair.
	/// If this is a leaf, the value is undefined.
	unsigned childIndex : 30;
	union
	{
		float splitPos; ///< If this is an inner node, specifies the position along the cardinal axis of the split.
		/// If this is a leaf, specifies the index/ofset to the object bucket for this leaf.
		/// If zero, this leaf does not have a bucket associated with it. (empty leaf)
		u32 bucketIndex;
	};

	/// If true, this leaf does not contain any objects.
	bool IsEmptyLeaf() const { assert(IsLeaf()); return bucketIndex == 0; }
	bool IsLeaf() const { return splitAxis == AxisNone; }
	int LeftChildIndex() const { return (int)childIndex; }
	int RightChildIndex() const { return (int)childIndex+1; }
	CardinalAxis SplitAxis() const { return (CardinalAxis)splitAxis; }
};

/// Type T must have a member function bool T.Intersects(const AABB &) const;
template<typename T>
class KdTree
{
public:
	/// Constructs an empty kD-tree.
	KdTree() {}

	~KdTree();

    void Clear();
    
	/// Adds a given number of objects to this kD-tree.
	/// Call this function repeatedly as many times as necessary to prepare the data. Then
	/// call Build() to create the tree data structure.
	void AddObjects(const T *objects, int numObjects);

	/// Creates the kD-tree data structure based on all the objects added to the tree.
	/// After Build() has been called, do *not* call AddObjects() again.
	void Build();

	/// Returns an object bucket by the given bucket index.
	/// An object bucket is a contiguous C array of object indices, terminated with a sentinel value BUCKET_SENTINEL.
	/// To fetch the actual object based on an object index, call the Object() method.
	u32 *Bucket(int bucketIndex);
	const u32 *Bucket(int bucketIndex) const;

	/// Returns an object by the given object index.
	T &Object(int objectIndex);
	const T &Object(int objectIndex) const;

	/// Returns the total number of nodes (all nodes, i.e. inner nodes + leaves) in the tree.
	int NumNodes() const;

	/// Returns the total number of leaf nodes in the tree.
	int NumLeaves() const;

	/// Returns the total number of inner nodes in the tree.
	int NumInnerNodes() const;

    /// Returns the total number of objects added to this container.
    int NumObjects() const;

	/// Returns the maximum height of the tree (the path from the root to the farthest leaf node).
	int TreeHeight() const;

	/// Returns the root node.
	KdTreeNode *Root();
	const KdTreeNode *Root() const;

	/// Returns true if the given node belongs to this kD-tree data structure.
	/// Use only for debugging!
	bool IsPartOfThisTree(const KdTreeNode *node) const;

	bool IsPartOfThisTree(const KdTreeNode *root, const KdTreeNode *node) const;

	/// Returns an AABB that tightly encloses all geometry in this kD-tree. Calling this is only valid after
	/// Build() has been called.
	const AABB &BoundingAABB() const { return rootAABB; }

	/// Traverses a ray through this kD-tree, and calls the given leafCallback function for each leaf of the tree.
	/// Uses the "recursive B" method from Vlastimil Havran's thesis.
	/** @param r The ray to query through this kD-tree.
		@param leafCallback A function or a function object of prototype
			bool LeafCallbackFunction(KdTree<T> &tree, const KdTreeNode &leaf, const Ray &ray, float tNear, float tFar);
			If the callback function returns true, the execution of the query is stopped and this function immediately
			returns afterwards. If the callback function returns false, the execution of the query continues. */
	template<typename Func>
	inline void RayQuery(const Ray &r, Func &leafCallback);

	/// Performs an AABB intersection query in this kD-tree, and calls the given leafCallback function for each leaf
	/// of the tree which intersects the given AABB.
	/** @param aabb The axis-aligned bounding box to query through this kD-tree.
		@param leafCallback A function or a function object of prototype
			bool LeafCallbackFunction(KdTree<T> &tree, KdTreeNode &leaf, const AABB &aabb);
			If the callback function returns true, the execution of the query is stopped and this function immediately
			returns afterwards. If the callback function returns false, the execution of the query continues. */
	template<typename Func>
	inline void AABBQuery(const AABB &aabb, Func &leafCallback);

#if 0 ///\bug Doesn't work properly. Fix up!
	/// Performs an intersection query of this kD-tree against a given kD-tree, and calls the given
	/// leafCallback function for each leaf pair that intersect each other.
	/// @param leafCallback A function or a function object of prototype
	///    bool LeafCallbackFunction(KdTree<T> &thisTree, KdTreeNode &thisLeaf, const AABB &thisLeafAABB,
	///                              KdTree<T> &tree2, KdTreeNode &tree2Leaf, const OBB &tree2LeafOBB);
	///    If the callback function returns true, the execution of the query is stopped and this function immediately
	///    returns afterwards. If the callback function returns false, the execution of the query continues.
	template<typename Func>
	inline void KdTreeQuery(KdTree<T> &tree2, const float3x4 &thisWorldTransform, const float3x4 &tree2WorldTransform, Func &leafCallback);
#endif

#ifdef MATH_CONTAINERLIB_SUPPORT
	/// Performs a nearest neighbor search on this kD-tree.
	/// @param leafCallback A function or a function object of prototype
	///    bool LeafCallbackFunction(KdTree<T> &tree, const float3 &point, KdTreeNode &leaf, const AABB &aabb, float minDistance);
	///    If the callback function returns true, the execution of the query is stopped and this function immediately
	///    returns afterwards. If the callback function returns false, the execution of the query continues.
	///	   minDistance is the minimum distance the objects in this leaf (and all future leaves to be passed to the
	///    callback) have to the point that is being queried.
	template<typename Func>
	inline void NearestObjects(const float3 &point, Func &leafCallback);
#endif

	static const u32 BUCKET_SENTINEL = 0xFFFFFFFF;

private:
	static const int maxNodes = 256 * 1024;
	static const int maxTreeDepth = 30;

	std::vector<KdTreeNode> nodes;
	std::vector<T> objects;
	std::vector<u32*> buckets;

	int AllocateNodePair();

	void FreeBuckets();

	AABB BoundingAABB(const u32 *bucket) const;

	void SplitLeaf(int nodeIndex, const AABB &nodeAABB, int numObjectsInBucket, int leafDepth);

	///\todo Implement support for deep copying.
	KdTree(const KdTree &);
	void operator =(const KdTree &);

	AABB rootAABB;

	int TreeHeight(int nodeIndex) const;
};

MATH_END_NAMESPACE

#include "KdTree.inl"

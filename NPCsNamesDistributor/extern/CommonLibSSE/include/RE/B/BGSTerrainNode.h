#pragma once

namespace RE
{
	class BGSTerrainManager;

	class BGSTerrainNode
	{
	public:
		enum class Flag
		{
			kLandHgtAltered = 0x1,
			kLandClrAltered = 0x2,
			kLandTexAltered = 0x4,
			kLandLoaded = 0x8,
			kShaderLandTextureCount = 0x8,
			kLandGoodNormals = 0x10,
			kBlocksize = 0x10,
			kLandHiresHeightfield = 0x20,
			kLandSize = 0x21,
			kLandDataAltered = 0x27,
			kTilesPerBlock = 0x100,
			kTrisPerBlock = 0x200,
			kTriStripIndexCount = 0x3FD,  // TRISTRIPINDEXCOUNT
			kLandRemapped = 0x400,
			kLandArea = 0x441,
			kHalfLand = 0x800,
			kFullLand = 0x1000,
		};

		std::uint32_t GetLODLevel() const { return (nodeState.underlying() >> 21) & 0x3FC; }

		// members
		BGSTerrainManager*                manager;         // 00
		void*                             chunkHandle;     // 08 - BSResource::RHandleType<BSResource::Entry<BGSTerrainChunk*,BSResource::EntryDBTraits<BGSBtrDB::DBTraits,BSResource::EntryDB<BGSBtrDB::DBTraits> >::CArgs>,BSResource::EntryDB<BGSBtrDB::DBTraits>>
		void*                             blockHandle;     // 10 - BSResource::RHandleType<BSResource::Entry<BGSDistantObjectBlock *,BSResource::EntryDBTraits<BGSBtoDB::DBTraits,BSResource::EntryDB<BGSBtoDB::DBTraits> >::CArgs>,BSResource::EntryDB<BGSBtoDB::DBTraits>>
		void*                             treeHandle;      // 18 - BSResource::RHandleType<BSResource::Entry<BGSDistantTreeBlock *,BSResource::EntryDBTraits<BGSBttDB::DBTraits,BSResource::EntryDB<BGSBttDB::DBTraits> >::CArgs>,BSResource::EntryDB<BGSBttDB::DBTraits>>
		void*                             mapChunkHandle;  // 20 - BSResource::RHandleType<BSResource::Entry<BGSTerrainChunk *,BSResource::EntryDBTraits<BGSBtrDB::DBTraits,BSResource::EntryDB<BGSBtrDB::DBTraits> >::CArgs>,BSResource::EntryDB<BGSBtrDB::DBTraits>>
		void*                             mapBlockHandle;  // 28 - BSResource::RHandleType<BSResource::Entry<BGSDistantObjectBlock *,BSResource::EntryDBTraits<BGSBtoDB::DBTraits,BSResource::EntryDB<BGSBtoDB::DBTraits> >::CArgs>,BSResource::EntryDB<BGSBtoDB::DBTraits>>
		BGSTerrainNode*                   children;        // 30
		BGSTerrainNode*                   parent;          // 38
		REX::EnumSet<Flag, std::uint32_t> nodeState;       // 40
		std::uint32_t                     nodeNumber;      // 44
		std::int16_t                      baseCellX;       // 48
		std::int16_t                      baseCellY;       // 4A
		std::uint32_t                     pad4C;           // 4C
	};
	static_assert(sizeof(BGSTerrainNode) == 0x50);
}

/*
* Copyright (c) 2006-2009 Erin Catto http://www.box2d.org
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*/

#include <Box2D/Common/b2BlockAllocator.h>
#include <limits.h>
#include <string.h>
#include <stddef.h>

using namespace b2d11;

int32 BlockAllocator::s_blockSizes[BLOCK_SIZES] = 
{
	16,		// 0
	32,		// 1
	64,		// 2
	96,		// 3
	128,	// 4
	160,	// 5
	192,	// 6
	224,	// 7
	256,	// 8
	320,	// 9
	384,	// 10
	448,	// 11
	512,	// 12
	640,	// 13
};
uint8 BlockAllocator::s_blockSizeLookup[MAX_BLOCK_SIZE + 1];
bool BlockAllocator::s_blockSizeLookupInitialized;

BlockAllocator::BlockAllocator()
{
	Assert(BLOCK_SIZES < UCHAR_MAX);

	m_chunkSpace = CHUNK_ARRAY_INCREMENT;
	m_chunkCount = 0;
	m_chunks = (Chunk*)Alloc(m_chunkSpace * sizeof(Chunk));
	
	memset(m_chunks, 0, m_chunkSpace * sizeof(Chunk));
	memset(m_freeLists, 0, sizeof(m_freeLists));

	if (s_blockSizeLookupInitialized == false)
	{
		int32 j = 0;
		for (int32 i = 1; i <= MAX_BLOCK_SIZE; ++i)
		{
			Assert(j < BLOCK_SIZES);
			if (i <= s_blockSizes[j])
			{
				s_blockSizeLookup[i] = (uint8)j;
			}
			else
			{
				++j;
				s_blockSizeLookup[i] = (uint8)j;
			}
		}

		s_blockSizeLookupInitialized = true;
	}
}

BlockAllocator::~BlockAllocator()
{
	for (int32 i = 0; i < m_chunkCount; ++i)
	{
		b2d11::Free(m_chunks[i].blocks);
	}

	b2d11::Free(m_chunks);
}

void* BlockAllocator::Allocate(int32 size)
{
	if (size == 0)
		return NULL;

	Assert(0 < size);

	if (size > MAX_BLOCK_SIZE)
	{
		return Alloc(size);
	}

	int32 index = s_blockSizeLookup[size];
	Assert(0 <= index && index < BLOCK_SIZES);

	if (m_freeLists[index])
	{
		Block* block = m_freeLists[index];
		m_freeLists[index] = block->next;
		return block;
	}
	else
	{
		if (m_chunkCount == m_chunkSpace)
		{
			Chunk* oldChunks = m_chunks;
			m_chunkSpace += CHUNK_ARRAY_INCREMENT;
			m_chunks = (Chunk*)Alloc(m_chunkSpace * sizeof(Chunk));
			memcpy(m_chunks, oldChunks, m_chunkCount * sizeof(Chunk));
			memset(m_chunks + m_chunkCount, 0, CHUNK_ARRAY_INCREMENT * sizeof(Chunk));
			b2d11::Free(oldChunks);
		}

		Chunk* chunk = m_chunks + m_chunkCount;
		chunk->blocks = (Block*)Alloc(CHUNK_SIZE);
#if defined(_DEBUG)
		memset(chunk->blocks, 0xcd, CHUNK_SIZE);
#endif
		int32 blockSize = s_blockSizes[index];
		chunk->blockSize = blockSize;
		int32 blockCount = CHUNK_SIZE / blockSize;
		Assert(blockCount * blockSize <= CHUNK_SIZE);
		for (int32 i = 0; i < blockCount - 1; ++i)
		{
			Block* block = (Block*)((int8*)chunk->blocks + blockSize * i);
			Block* next = (Block*)((int8*)chunk->blocks + blockSize * (i + 1));
			block->next = next;
		}
		Block* last = (Block*)((int8*)chunk->blocks + blockSize * (blockCount - 1));
		last->next = NULL;

		m_freeLists[index] = chunk->blocks->next;
		++m_chunkCount;

		return chunk->blocks;
	}
}

void BlockAllocator::Free(void* p, int32 size)
{
	if (size == 0)
	{
		return;
	}

	Assert(0 < size);

	if (size > MAX_BLOCK_SIZE)
	{
		b2d11::Free(p);
		return;
	}

	int32 index = s_blockSizeLookup[size];
	Assert(0 <= index && index < BLOCK_SIZES);

#ifdef _DEBUG
	// Verify the memory address and size is valid.
	int32 blockSize = s_blockSizes[index];
	bool found = false;
	for (int32 i = 0; i < m_chunkCount; ++i)
	{
		Chunk* chunk = m_chunks + i;
		if (chunk->blockSize != blockSize)
		{
			Assert(	(int8*)p + blockSize <= (int8*)chunk->blocks ||
						(int8*)chunk->blocks + CHUNK_SIZE <= (int8*)p);
		}
		else
		{
			if ((int8*)chunk->blocks <= (int8*)p && (int8*)p + blockSize <= (int8*)chunk->blocks + CHUNK_SIZE)
			{
				found = true;
			}
		}
	}

	Assert(found);

	memset(p, 0xfd, blockSize);
#endif

	Block* block = (Block*)p;
	block->next = m_freeLists[index];
	m_freeLists[index] = block;
}

void BlockAllocator::Clear()
{
	for (int32 i = 0; i < m_chunkCount; ++i)
	{
		b2d11::Free(m_chunks[i].blocks);
	}

	m_chunkCount = 0;
	memset(m_chunks, 0, m_chunkSpace * sizeof(Chunk));

	memset(m_freeLists, 0, sizeof(m_freeLists));
}

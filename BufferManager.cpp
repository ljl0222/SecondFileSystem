
#include "BufferManager.h"
#include "User.h"
#include <iostream>
#include <cstring>

extern User globalUser;
extern DeviceManager globalDeviceManager;
//extern BufferManager globalBufferManager;
//extern OpenFileTable globalOpenFileTable;
//extern SuperBlock globalSuperBlock;
//extern FileSystem globalFileSystem;
//extern InodeTable globalINodeTable;
//extern FileManager globalFileManager;

using namespace std;


void BufferManager::Initialize()
{
	for (int i = 0; i < NBUF; ++i) 
	{
		if (i) 
		{
			m_Buf[i].b_forw = m_Buf + i - 1;
		}
		else 
		{
			m_Buf[i].b_forw = bFreeList;
			bFreeList->b_back = m_Buf + i;
		}

		if (i + 1 < NBUF) 
		{
			m_Buf[i].b_back = m_Buf + i + 1;
		}
		else 
		{
			m_Buf[i].b_back = bFreeList;
			bFreeList->b_forw = m_Buf + i;
		}
		m_Buf[i].b_addr = Buffer[i];
	}
	return;
}

Buf* BufferManager::GetBlk(int blkno)
{
	Buf* bp;
	// User& u = globalUser;

	if (m_map.find(blkno) != m_map.end())
	{
		bp = m_map[blkno];
		Bdetach(bp);
		return bp;
	}

	// 否则分配自由队列
	bp = bFreeList->b_back;
	if (bp == bFreeList)
	{
		cout << "bFreeList队列不足" << endl;
		return NULL;
	}

	Bdetach(bp);
	m_map.erase(bp->b_blkno);

	if (bp->b_flags & Buf::B_DELWRI) {
		m_DeviceManager->write(bp->b_addr, BUFFER_SIZE, bp->b_blkno * BUFFER_SIZE);
	}

	bp->b_flags &= ~(Buf::B_DELWRI | Buf::B_DONE);
	bp->b_blkno = blkno;
	m_map[blkno] = bp;

	return bp;
}

void BufferManager::Brelse(Buf* bp)
{
	Binsert(bp);
	return;
}

Buf* BufferManager::Bread(int blkno)
{
	Buf* bp;
	bp = this->GetBlk(blkno);
	if (bp->b_flags & (Buf::B_DONE | Buf::B_DELWRI))
	{
		return bp;
	}

	m_DeviceManager->read(bp->b_addr, BUFFER_SIZE, bp->b_blkno * BUFFER_SIZE);

	bp->b_flags |= Buf::B_DONE;
	
	return bp;
}

void BufferManager::Bwrite(Buf* bp)
{
	bp->b_flags &= ~(Buf::B_DELWRI);
	m_DeviceManager->write(bp->b_addr, BUFFER_SIZE, bp->b_blkno * BUFFER_SIZE);
	bp->b_flags |= Buf::B_DONE;
	Brelse(bp);
	return;
}

void BufferManager::Bformat()
{
	Buf b;
	for (int cnt = 0; cnt < NBUF; cnt++) 
	{
		memcpy(m_Buf + cnt, &b, sizeof(Buf));
	}
	Initialize();
}

void BufferManager::Bdwrite(Buf* bp)
{
	/* 置上B_DONE允许其它进程使用该磁盘块内容 */
	bp->b_flags |= (Buf::B_DELWRI | Buf::B_DONE);
	this->Brelse(bp);
	return;
}


void BufferManager::ClrBuf(Buf* bp)
{
	memset(bp->b_addr, 0, BUFFER_SIZE);
	return;
}

void BufferManager::Bflush()
{
	Buf* bp = NULL;

	for (int cnt = 0; cnt < NBUF; cnt++) 
	{
		bp = m_Buf + cnt;
		if (bp->b_flags & Buf::B_DELWRI) {
			bp->b_flags &= ~(Buf::B_DELWRI);
			m_DeviceManager->write(bp->b_addr, BUFFER_SIZE, bp->b_blkno * BUFFER_SIZE);
			bp->b_flags |= (Buf::B_DONE);
		}
	}
	return;
}

BufferManager::BufferManager()
{
	bFreeList = new Buf;
	Initialize();
	m_DeviceManager = &globalDeviceManager;
}

BufferManager::~BufferManager()
{
	Bflush();
	delete bFreeList;
}

// LRU

void BufferManager::Bdetach(Buf *bp)
{
	if (bp->b_back == NULL) {
		return;
	}

	bp->b_forw->b_back = bp->b_back;
	bp->b_back->b_forw = bp->b_forw;

	bp->b_back = NULL;
	bp->b_forw = NULL;
}

void BufferManager::Binsert(Buf* bp)
{
	if (bp->b_back != NULL) {
		return;
	}

	bp->b_forw = bFreeList->b_forw;
	bp->b_back = bFreeList;

	bFreeList->b_forw->b_back = bp;
	bFreeList->b_forw = bp;
}
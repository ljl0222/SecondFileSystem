#include "FileSystem.h"
#include "User.h"
#include <time.h>
#include <iostream>
extern User globalUser;
extern DeviceManager globalDeviceManager;
extern BufferManager globalBufferManager;
//extern OpenFileTable globalOpenFileTable;
extern SuperBlock globalSuperBlock;
//extern FileSystem globalFileSystem;
extern InodeTable globalINodeTable;
extern FileManager globalFileManager;

using namespace std;

/*==============================class SuperBlock===================================*/

SuperBlock::SuperBlock()
{
	//nothing to do here
}

SuperBlock::~SuperBlock()
{
	//nothing to do here
}

/*==============================class FileSystem===================================*/
FileSystem::FileSystem()
{
	m_DeviceManager = &globalDeviceManager;
	m_SuperBlock = &globalSuperBlock;
	m_BufferManager = &globalBufferManager;

	if (!m_DeviceManager->Exists())
		FormatFileSystem();
	else
		LoadSuperBlock();
}

FileSystem::~FileSystem()
{
	Update();
	m_DeviceManager = NULL;
	m_SuperBlock = NULL;
}


void FileSystem::LoadSuperBlock()
{
	m_DeviceManager->read(m_SuperBlock, sizeof(SuperBlock), 0);
}

void FileSystem::Update()
{
	SuperBlock* sb;
	Buf* pBuf;

	m_SuperBlock->s_fmod = 0;
	m_SuperBlock->s_time = (int)time(0);

	for (int cnt = 0; cnt < 2; cnt++) {
		int* p = (int*)m_SuperBlock + cnt * 128;
		pBuf = this->m_BufferManager->GetBlk(0 + cnt);
		memcpy(pBuf->b_addr, p, 512);
		m_BufferManager->Bwrite(pBuf);
	}

	globalINodeTable.UpdateInodeTable();
	m_BufferManager->Bflush();
}

Inode* FileSystem::IAlloc()
{
	Buf* pBuf;
	Inode* pNode;
	User& u = globalUser;
	int ino;	/* 分配到的空闲外存Inode编号 */

	/*
	 * SuperBlock直接管理的空闲Inode索引表已空，
	 * 必须到磁盘上搜索空闲Inode。先对inode列表上锁，
	 * 因为在以下程序中会进行读盘操作可能会导致进程切换，
	 * 其他进程有可能访问该索引表，将会导致不一致性。
	 */
	if (m_SuperBlock->s_ninode <= 0)
	{
		/* 外存Inode编号从0开始，这不同于Unix V6中外存Inode从1开始编号 */
		ino = -1;

		/* 依次读入磁盘Inode区中的磁盘块，搜索其中空闲外存Inode，记入空闲Inode索引表 */
		for (int i = 0; i < m_SuperBlock->s_isize; i++)
		{
			pBuf = this->m_BufferManager->Bread(FileSystem::INODE_ZONE_START_SECTOR + i);

			/* 获取缓冲区首址 */
			int* p = (int*)pBuf->b_addr;

			/* 检查该缓冲区中每个外存Inode的i_mode != 0，表示已经被占用 */
			for (int j = 0; j < FileSystem::INODE_NUMBER_PER_SECTOR; j++)
			{
				ino++;

				int mode = *(p + j * sizeof(DiskInode) / sizeof(int));

				/* 该外存Inode已被占用，不能记入空闲Inode索引表 */
				if (mode != 0)
				{
					continue;
				}

				/*
				 * 如果外存inode的i_mode==0，此时并不能确定
				 * 该inode是空闲的，因为有可能是内存inode没有写到
				 * 磁盘上,所以要继续搜索内存inode中是否有相应的项
				 */
				if (globalINodeTable.IsLoaded(ino) == -1)
				{
					/* 该外存Inode没有对应的内存拷贝，将其记入空闲Inode索引表 */
					m_SuperBlock->s_inode[m_SuperBlock->s_ninode++] = ino;

					/* 如果空闲索引表已经装满，则不继续搜索 */
					if (m_SuperBlock->s_ninode >= 100)
					{
						break;
					}
				}
			}

			/* 至此已读完当前磁盘块，释放相应的缓存 */
			this->m_BufferManager->Brelse(pBuf);

			/* 如果空闲索引表已经装满，则不继续搜索 */
			if (m_SuperBlock->s_ninode >= 100)
			{
				break;
			}
		}

		/* 如果在磁盘上没有搜索到任何可用外存Inode，返回NULL */
		if (m_SuperBlock->s_ninode <= 0)
		{
			u.u_error = User::myENOSPC;
			return NULL;
		}
	}
	
	ino = m_SuperBlock->s_inode[--m_SuperBlock->s_ninode];
	pNode = globalINodeTable.IGet(ino);
	if (NULL == pNode) {
		cout << "没有空闲内存的Inode" << endl;
		return NULL;
	}

	pNode->Clean();
	m_SuperBlock->s_fmod = 1;
	return pNode;
}

void FileSystem::IFree(int number)
{
	/*
	 * 如果超级块直接管理的空闲外存Inode超过100个，
	 * 同样让释放的外存Inode散落在磁盘Inode区中。
	 */
	if (m_SuperBlock->s_ninode >= 100)
	{
		return;
	}

	m_SuperBlock->s_inode[m_SuperBlock->s_ninode++] = number;

	/* 设置SuperBlock被修改标志 */
	m_SuperBlock->s_fmod = 1;
}

Buf* FileSystem::Alloc()
{
	int blkno;	/* 分配到的空闲磁盘块编号 */
	Buf* pBuf;
	User& u = globalUser;

	/* 从索引表“栈顶”获取空闲磁盘块编号 */
	blkno = m_SuperBlock->s_free[--m_SuperBlock->s_nfree];

	/*
	 * 若获取磁盘块编号为零，则表示已分配尽所有的空闲磁盘块。
	 * 或者分配到的空闲磁盘块编号不属于数据盘块区域中(由BadBlock()检查)，
	 * 都意味着分配空闲磁盘块操作失败。
	 */
	if (0 >= blkno)
	{
		m_SuperBlock->s_nfree = 0;
		u.u_error = User::myENOSPC;
		return NULL;
	}

	/*
	 * 栈已空，新分配到空闲磁盘块中记录了下一组空闲磁盘块的编号,
	 * 将下一组空闲磁盘块的编号读入SuperBlock的空闲磁盘块索引表s_free[100]中。
	 */
	if (m_SuperBlock->s_nfree <= 0)
	{
		/* 读入该空闲磁盘块 */
		pBuf = this->m_BufferManager->Bread(blkno);

		/* 从该磁盘块的0字节开始记录，共占据4(s_nfree)+400(s_free[100])个字节 */
		int* p = (int*)pBuf->b_addr;

		/* 首先读出空闲盘块数s_nfree */
		m_SuperBlock->s_nfree = *p++;

		/* 读取缓存中后续位置的数据，写入到SuperBlock空闲盘块索引表s_free[100]中 */
		memcpy(m_SuperBlock->s_free, p, sizeof(m_SuperBlock->s_free));

		/* 缓存使用完毕，释放以便被其它进程使用 */
		this->m_BufferManager->Brelse(pBuf);
	}

	/* 普通情况下成功分配到一空闲磁盘块 */
	pBuf = this->m_BufferManager->GetBlk(blkno);	/* 为该磁盘块申请缓存 */
	if (pBuf) {
		this->m_BufferManager->ClrBuf(pBuf);	/* 清空缓存中的数据 */
	}
	m_SuperBlock->s_fmod = 1;	/* 设置SuperBlock被修改标志 */

	return pBuf;
}

void FileSystem::Free(int blkno)
{
	Buf* pBuf;
	User& u = globalUser;


	/* SuperBlock中直接管理空闲磁盘块号的栈已满 */
	if (m_SuperBlock->s_nfree >= 100)
	{
		/*
		 * 使用当前Free()函数正要释放的磁盘块，存放前一组100个空闲
		 * 磁盘块的索引表
		 */
		pBuf = this->m_BufferManager->GetBlk(blkno);	/* 为当前正要释放的磁盘块分配缓存 */

		/* 从该磁盘块的0字节开始记录，共占据4(s_nfree)+400(s_free[100])个字节 */
		int* p = (int*)pBuf->b_addr;

		/* 首先写入空闲盘块数，除了第一组为99块，后续每组都是100块 */
		*p++ = m_SuperBlock->s_nfree;
		/* 将SuperBlock的空闲盘块索引表s_free[100]写入缓存中后续位置 */
		memcpy(p, m_SuperBlock->s_free, sizeof(int) * 100);

		m_SuperBlock->s_nfree = 0;
		/* 将存放空闲盘块索引表的“当前释放盘块”写入磁盘，即实现了空闲盘块记录空闲盘块号的目标 */
		this->m_BufferManager->Bwrite(pBuf);
	}
	m_SuperBlock->s_free[m_SuperBlock->s_nfree++] = blkno;	/* SuperBlock中记录下当前释放盘块号 */
	m_SuperBlock->s_fmod = 1;
}


void FileSystem::FormatSuperBlock()
{
	m_SuperBlock->s_isize = FileSystem::INODE_ZONE_SIZE;


	m_SuperBlock->s_fsize = FileSystem::DISK_SIZE;


	m_SuperBlock->s_nfree = 0;
	m_SuperBlock->s_free[0] = -1;
	m_SuperBlock->s_ninode = 0;
	m_SuperBlock->s_flock = 0;
	m_SuperBlock->s_ilock = 0;
	m_SuperBlock->s_fmod = 0;
	m_SuperBlock->s_ronly = 0;

	time((time_t*)&m_SuperBlock->s_time); // 更新访问时间
}

void FileSystem::FormatFileSystem()
{
	FormatSuperBlock();
	m_DeviceManager->Construct();

	m_DeviceManager->write(m_SuperBlock, sizeof(SuperBlock), 0);

	DiskInode root;
	root.d_mode |= Inode::IALLOC | Inode::IFDIR;
	root.d_nlink = 1;

	m_DeviceManager->write(&root, sizeof(root));

	DiskInode empty;
	for (int cnt = 1; cnt < 8176; cnt++) {
		if (m_SuperBlock->s_ninode < 100) {
			m_SuperBlock->s_inode[m_SuperBlock->s_ninode++] = cnt;
		}
		m_DeviceManager->write(&empty, sizeof(empty));
	}

	char freeBlock[512], tmpFreeBlock[512];
	memset(freeBlock, 0, sizeof(freeBlock));
	memset(tmpFreeBlock, 0, sizeof(tmpFreeBlock));

	for (int cnt = 0; cnt < FileSystem::DATA_ZONE_SIZE; cnt++) {
		if (m_SuperBlock->s_nfree >= 100) {
			memcpy(tmpFreeBlock, &m_SuperBlock->s_nfree, sizeof(m_SuperBlock->s_free) + sizeof(int));
			m_DeviceManager->write(tmpFreeBlock, 512);
			m_SuperBlock->s_nfree = 0;
		}
		else {
			m_DeviceManager->write(freeBlock, 512);
		}
		m_SuperBlock->s_free[m_SuperBlock->s_nfree++] = cnt + DATA_ZONE_START_SECTOR;
	}

	time((time_t*)&m_SuperBlock->s_time);

	m_DeviceManager->write(m_SuperBlock, sizeof(SuperBlock), 0);
}
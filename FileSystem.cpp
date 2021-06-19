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
	int ino;	/* ���䵽�Ŀ������Inode��� */

	/*
	 * SuperBlockֱ�ӹ���Ŀ���Inode�������ѿգ�
	 * ���뵽��������������Inode���ȶ�inode�б�������
	 * ��Ϊ�����³����л���ж��̲������ܻᵼ�½����л���
	 * ���������п��ܷ��ʸ����������ᵼ�²�һ���ԡ�
	 */
	if (m_SuperBlock->s_ninode <= 0)
	{
		/* ���Inode��Ŵ�0��ʼ���ⲻͬ��Unix V6�����Inode��1��ʼ��� */
		ino = -1;

		/* ���ζ������Inode���еĴ��̿飬�������п������Inode���������Inode������ */
		for (int i = 0; i < m_SuperBlock->s_isize; i++)
		{
			pBuf = this->m_BufferManager->Bread(FileSystem::INODE_ZONE_START_SECTOR + i);

			/* ��ȡ��������ַ */
			int* p = (int*)pBuf->b_addr;

			/* ���û�������ÿ�����Inode��i_mode != 0����ʾ�Ѿ���ռ�� */
			for (int j = 0; j < FileSystem::INODE_NUMBER_PER_SECTOR; j++)
			{
				ino++;

				int mode = *(p + j * sizeof(DiskInode) / sizeof(int));

				/* �����Inode�ѱ�ռ�ã����ܼ������Inode������ */
				if (mode != 0)
				{
					continue;
				}

				/*
				 * ������inode��i_mode==0����ʱ������ȷ��
				 * ��inode�ǿ��еģ���Ϊ�п������ڴ�inodeû��д��
				 * ������,����Ҫ���������ڴ�inode���Ƿ�����Ӧ����
				 */
				if (globalINodeTable.IsLoaded(ino) == -1)
				{
					/* �����Inodeû�ж�Ӧ���ڴ濽��������������Inode������ */
					m_SuperBlock->s_inode[m_SuperBlock->s_ninode++] = ino;

					/* ��������������Ѿ�װ�����򲻼������� */
					if (m_SuperBlock->s_ninode >= 100)
					{
						break;
					}
				}
			}

			/* �����Ѷ��굱ǰ���̿飬�ͷ���Ӧ�Ļ��� */
			this->m_BufferManager->Brelse(pBuf);

			/* ��������������Ѿ�װ�����򲻼������� */
			if (m_SuperBlock->s_ninode >= 100)
			{
				break;
			}
		}

		/* ����ڴ�����û���������κο������Inode������NULL */
		if (m_SuperBlock->s_ninode <= 0)
		{
			u.u_error = User::myENOSPC;
			return NULL;
		}
	}
	
	ino = m_SuperBlock->s_inode[--m_SuperBlock->s_ninode];
	pNode = globalINodeTable.IGet(ino);
	if (NULL == pNode) {
		cout << "û�п����ڴ��Inode" << endl;
		return NULL;
	}

	pNode->Clean();
	m_SuperBlock->s_fmod = 1;
	return pNode;
}

void FileSystem::IFree(int number)
{
	/*
	 * ���������ֱ�ӹ���Ŀ������Inode����100����
	 * ͬ�����ͷŵ����Inodeɢ���ڴ���Inode���С�
	 */
	if (m_SuperBlock->s_ninode >= 100)
	{
		return;
	}

	m_SuperBlock->s_inode[m_SuperBlock->s_ninode++] = number;

	/* ����SuperBlock���޸ı�־ */
	m_SuperBlock->s_fmod = 1;
}

Buf* FileSystem::Alloc()
{
	int blkno;	/* ���䵽�Ŀ��д��̿��� */
	Buf* pBuf;
	User& u = globalUser;

	/* ��������ջ������ȡ���д��̿��� */
	blkno = m_SuperBlock->s_free[--m_SuperBlock->s_nfree];

	/*
	 * ����ȡ���̿���Ϊ�㣬���ʾ�ѷ��価���еĿ��д��̿顣
	 * ���߷��䵽�Ŀ��д��̿��Ų����������̿�������(��BadBlock()���)��
	 * ����ζ�ŷ�����д��̿����ʧ�ܡ�
	 */
	if (0 >= blkno)
	{
		m_SuperBlock->s_nfree = 0;
		u.u_error = User::myENOSPC;
		return NULL;
	}

	/*
	 * ջ�ѿգ��·��䵽���д��̿��м�¼����һ����д��̿�ı��,
	 * ����һ����д��̿�ı�Ŷ���SuperBlock�Ŀ��д��̿�������s_free[100]�С�
	 */
	if (m_SuperBlock->s_nfree <= 0)
	{
		/* ����ÿ��д��̿� */
		pBuf = this->m_BufferManager->Bread(blkno);

		/* �Ӹô��̿��0�ֽڿ�ʼ��¼����ռ��4(s_nfree)+400(s_free[100])���ֽ� */
		int* p = (int*)pBuf->b_addr;

		/* ���ȶ��������̿���s_nfree */
		m_SuperBlock->s_nfree = *p++;

		/* ��ȡ�����к���λ�õ����ݣ�д�뵽SuperBlock�����̿�������s_free[100]�� */
		memcpy(m_SuperBlock->s_free, p, sizeof(m_SuperBlock->s_free));

		/* ����ʹ����ϣ��ͷ��Ա㱻��������ʹ�� */
		this->m_BufferManager->Brelse(pBuf);
	}

	/* ��ͨ����³ɹ����䵽һ���д��̿� */
	pBuf = this->m_BufferManager->GetBlk(blkno);	/* Ϊ�ô��̿����뻺�� */
	if (pBuf) {
		this->m_BufferManager->ClrBuf(pBuf);	/* ��ջ����е����� */
	}
	m_SuperBlock->s_fmod = 1;	/* ����SuperBlock���޸ı�־ */

	return pBuf;
}

void FileSystem::Free(int blkno)
{
	Buf* pBuf;
	User& u = globalUser;


	/* SuperBlock��ֱ�ӹ�����д��̿�ŵ�ջ���� */
	if (m_SuperBlock->s_nfree >= 100)
	{
		/*
		 * ʹ�õ�ǰFree()������Ҫ�ͷŵĴ��̿飬���ǰһ��100������
		 * ���̿��������
		 */
		pBuf = this->m_BufferManager->GetBlk(blkno);	/* Ϊ��ǰ��Ҫ�ͷŵĴ��̿���仺�� */

		/* �Ӹô��̿��0�ֽڿ�ʼ��¼����ռ��4(s_nfree)+400(s_free[100])���ֽ� */
		int* p = (int*)pBuf->b_addr;

		/* ����д������̿��������˵�һ��Ϊ99�飬����ÿ�鶼��100�� */
		*p++ = m_SuperBlock->s_nfree;
		/* ��SuperBlock�Ŀ����̿�������s_free[100]д�뻺���к���λ�� */
		memcpy(p, m_SuperBlock->s_free, sizeof(int) * 100);

		m_SuperBlock->s_nfree = 0;
		/* ����ſ����̿�������ġ���ǰ�ͷ��̿顱д����̣���ʵ���˿����̿��¼�����̿�ŵ�Ŀ�� */
		this->m_BufferManager->Bwrite(pBuf);
	}
	m_SuperBlock->s_free[m_SuperBlock->s_nfree++] = blkno;	/* SuperBlock�м�¼�µ�ǰ�ͷ��̿�� */
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

	time((time_t*)&m_SuperBlock->s_time); // ���·���ʱ��
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
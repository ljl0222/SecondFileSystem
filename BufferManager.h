#ifndef BUFFMANAGER_H

#define BUFFMANAGER_H

#include "Buf.h"
#include "DeviceManager.h"

#include <unordered_map>

using namespace std;

class BufferManager
{
public:
	/* static const member */
	static const int NBUF = 100;			/* ������ƿ顢������������ */
	static const int BUFFER_SIZE = 512;	/* ��������С�� ���ֽ�Ϊ��λ */

public:
	BufferManager();
	~BufferManager();

	void Initialize();	

	Buf* GetBlk(int blkno);	/* ����һ�黺�棬���ڶ�д�豸dev�ϵ��ַ���blkno��*/
	void Brelse(Buf* bp);				/* �ͷŻ�����ƿ�buf */

	Buf* Bread(int blkno);	/* ��һ�����̿顣devΪ�������豸�ţ�blknoΪĿ����̿��߼���š� */

	void Bwrite(Buf* bp);				/* дһ�����̿� */
	void Bdwrite(Buf* bp);				/* �ӳ�д���̿� */

	void ClrBuf(Buf* bp);				/* ��ջ��������� */
	void Bflush();				/* ��devָ���豸�������ӳ�д�Ļ���ȫ����������� */

	void Bformat();

	void Bdetach(Buf* bp);
	void Binsert(Buf* bp);

private:
	void GetError(Buf* bp);				/* ��ȡI/O�����з����Ĵ�����Ϣ */
	void NotAvail(Buf* bp);				/* �����ɶ�����ժ��ָ���Ļ�����ƿ�buf */
	Buf* InCore(short adev, int blkno);	/* ���ָ���ַ����Ƿ����ڻ����� */

private:
	Buf *bFreeList;						/* ���ɻ�����п��ƿ� */					
	Buf m_Buf[NBUF];					/* ������ƿ����� */
	unsigned char Buffer[NBUF][BUFFER_SIZE];	/* ���������� */
	DeviceManager* m_DeviceManager;		/* ָ���豸����ģ��ȫ�ֶ��� */
	unordered_map<int, Buf*> m_map;
};

#endif


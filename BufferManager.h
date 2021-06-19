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
	static const int NBUF = 100;			/* 缓存控制块、缓冲区的数量 */
	static const int BUFFER_SIZE = 512;	/* 缓冲区大小。 以字节为单位 */

public:
	BufferManager();
	~BufferManager();

	void Initialize();	

	Buf* GetBlk(int blkno);	/* 申请一块缓存，用于读写设备dev上的字符块blkno。*/
	void Brelse(Buf* bp);				/* 释放缓存控制块buf */

	Buf* Bread(int blkno);	/* 读一个磁盘块。dev为主、次设备号，blkno为目标磁盘块逻辑块号。 */

	void Bwrite(Buf* bp);				/* 写一个磁盘块 */
	void Bdwrite(Buf* bp);				/* 延迟写磁盘块 */

	void ClrBuf(Buf* bp);				/* 清空缓冲区内容 */
	void Bflush();				/* 将dev指定设备队列中延迟写的缓存全部输出到磁盘 */

	void Bformat();

	void Bdetach(Buf* bp);
	void Binsert(Buf* bp);

private:
	void GetError(Buf* bp);				/* 获取I/O操作中发生的错误信息 */
	void NotAvail(Buf* bp);				/* 从自由队列中摘下指定的缓存控制块buf */
	Buf* InCore(short adev, int blkno);	/* 检查指定字符块是否已在缓存中 */

private:
	Buf *bFreeList;						/* 自由缓存队列控制块 */					
	Buf m_Buf[NBUF];					/* 缓存控制块数组 */
	unsigned char Buffer[NBUF][BUFFER_SIZE];	/* 缓冲区数组 */
	DeviceManager* m_DeviceManager;		/* 指向设备管理模块全局对象 */
	unordered_map<int, Buf*> m_map;
};

#endif


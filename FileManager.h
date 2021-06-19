#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include "FileSystem.h"
#include "INode.h"
#include "OpenFileManager.h"
using namespace std;

/*
 * 文件管理类(FileManager)
 * 封装了文件系统的各种系统调用在核心态下处理过程，
 * 如对文件的Open()、Close()、Read()、Write()等等
 * 封装了对文件系统访问的具体细节。
 */
class FileManager
{
public:
	/* 目录搜索模式，用于NameI()函数 */
	enum DirectorySearchMode
	{
		OPEN = 0,		/* 以打开文件方式搜索目录 */
		CREATE = 1,		/* 以新建文件方式搜索目录 */
		DELETE = 2		/* 以删除文件方式搜索目录 */
	};

	/* Functions */
public:
	/* Constructors */
	FileManager();
	/* Destructors */
	~FileManager();


	/*
	 * @comment Open()系统调用处理过程
	 */
	void Open();

	/*
	 * @comment Creat()系统调用处理过程
	 */
	void Creat();

	/*
	 * @comment Open()、Creat()系统调用的公共部分
	 */
	void Open1(Inode* pInode, int mode, int trf);

	/*
	 * @comment Close()系统调用处理过程
	 */
	void Close();

	/*
	 * @comment Seek()系统调用处理过程
	 */
	void Seek();

	/*
	 * @comment Read()系统调用处理过程
	 */
	void Read();

	/*
	 * @comment Write()系统调用处理过程
	 */
	void Write();

	/*
	 * @comment 读写系统调用公共部分代码
	 */
	void Rdwr(enum File::FileFlags mode);

	/*
	 * @comment 目录搜索，将路径转化为相应的Inode，
	 * 返回上锁后的Inode
	 */
	Inode* NameI(enum DirectorySearchMode mode);

	/*
	 * @comment 被Creat()系统调用使用，用于为创建新文件分配内核资源
	 */
	Inode* MakNode(unsigned int mode);

	/*
	 * @comment 向父目录的目录文件写入一个目录项
	 */
	void WriteDir(Inode* pInode);


	/* 改变当前工作目录 */
	void ChDir();

	/* 取消文件 */
	void UnLink();

	void Ls();

public:
	/* 根目录内存Inode */
	Inode* rootDirInode;

	/* 对全局对象g_FileSystem的引用，该对象负责管理文件系统存储资源 */
	FileSystem* m_FileSystem;

	/* 对全局对象g_InodeTable的引用，该对象负责内存Inode表的管理 */
	InodeTable* m_InodeTable;

	/* 对全局对象g_OpenFileTable的引用，该对象负责打开文件表项的管理 */
	OpenFileTable* m_OpenFileTable;
};

#endif // !FILEMANAGER_H

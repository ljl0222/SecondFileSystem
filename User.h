#ifndef USER_H

#define USER_H


#include "FileManager.h"
#include <string>

using namespace std;
/*
 * @comment ������Unixv6�� struct user�ṹ��Ӧ�����ֻ�ı�
 * ���������޸ĳ�Ա�ṹ���֣������������͵Ķ�Ӧ��ϵ����:
 */
class User
{
public:
	static const int EAX = 0;	/* u.u_ar0[EAX]�������ֳ���������EAX�Ĵ�����ƫ���� */

	/* u_error's Error Code */
	/* 1~32 ����linux ���ں˴����е�/usr/include/asm/errno.h, ����for V6++ */
	enum ErrorCode
	{
		myNOERROR = 0,	/* No error */
		myEPERM = 1,	/* Operation not permitted */
		myENOENT = 2,	/* No such file or directory */
		myESRCH = 3,	/* No such process */
		myEINTR = 4,	/* Interrupted system call */
		myEIO = 5,	/* I/O error */
		myENXIO = 6,	/* No such device or address */
		myE2BIG = 7,	/* Arg list too long */
		myENOEXEC = 8,	/* Exec format error */
		myEBADF = 9,	/* Bad file number */
		myECHILD = 10,	/* No child processes */
		myEAGAIN = 11,	/* Try again */
		myENOMEM = 12,	/* Out of memory */
		myEACCES = 13,	/* Permission denied */
		myEFAULT = 14,	/* Bad address */
		myENOTBLK = 15,	/* Block device required */
		myEBUSY = 16,	/* Device or resource busy */
		myEEXIST = 17,	/* File exists */
		myEXDEV = 18,	/* Cross-device link */
		myENODEV = 19,	/* No such device */
		myENOTDIR = 20,	/* Not a directory */
		myEISDIR = 21,	/* Is a directory */
		myEINVAL = 22,	/* Invalid argument */
		myENFILE = 23,	/* File table overflow */
		myEMFILE = 24,	/* Too many open files */
		myENOTTY = 25,	/* Not a typewriter(terminal) */
		myETXTBSY = 26,	/* Text file busy */
		myEFBIG = 27,	/* File too large */
		myENOSPC = 28,	/* No space left on device */
		myESPIPE = 29,	/* Illegal seek */
		myEROFS = 30,	/* Read-only file system */
		myEMLINK = 31,	/* Too many links */
		myEPIPE = 32,	/* Broken pipe */
		myENOSYS = 100,
		//myEFAULT	= 106
	};


public:
	unsigned long u_rsav[2];	/* ���ڱ���esp��ebpָ�� */
	unsigned long u_ssav[2];	/* ���ڶ�esp��ebpָ��Ķ��α��� */

	/* ϵͳ������س�Ա */
	unsigned int u_ar0[1024];		/* ָ�����ջ�ֳ���������EAX�Ĵ���
								��ŵ�ջ��Ԫ�����ֶδ�Ÿ�ջ��Ԫ�ĵ�ַ��
								��V6��r0���ϵͳ���õķ���ֵ���û�����
								x86ƽ̨��ʹ��EAX��ŷ���ֵ�����u.u_ar0[R0] */

	int u_arg[5];				/* ��ŵ�ǰϵͳ���ò��� */
	string u_dirp;				/* ϵͳ���ò���(һ������Pathname)��ָ�� */


	/* �ļ�ϵͳ��س�Ա */
	Inode* u_cdir;		/* ָ��ǰĿ¼��Inodeָ�� */
	Inode* u_pdir;		/* ָ��Ŀ¼��Inodeָ�� */

	DirectoryEntry u_dent;					/* ��ǰĿ¼��Ŀ¼�� */
	char u_dbuf[DirectoryEntry::DIRSIZ];	/* ��ǰ·������������Ӧ��Ϊ DirectoryEntry::DIRSIZ */
	string u_curdir;						/* ��ǰ����Ŀ¼����·�� */

	ErrorCode u_error;			/* ��Ŵ����� */


	/* �ļ�ϵͳ��س�Ա */
	OpenFiles u_ofiles;		/* ���̴��ļ������������ */

	/* �ļ�I/O���� */
	IOParameter u_IOParam;	/* ��¼��ǰ����д�ļ���ƫ�������û�Ŀ�������ʣ���ֽ������� */

	FileManager* u_FileManager;
	string ls;
	/* Member Functions */
public:
	User();
	~User();
	/* ����ϵͳ���ò���uid������Ч�û�ID����ʵ�û�ID�������û�ID(p_uid) */
	void Setuid();

	/* ��ȡ�û�ID����16����Ϊ��ʵ�û�ID(u_ruid)����16����Ϊ��Ч�û�ID(u_uid) */
	void Getuid();

	/* ����ϵͳ���ò���gid������Ч��ID����ʵ��ID */
	void Setgid();

	/* ��ȡ��ID, ��16����Ϊ��ʵ��ID(u_rgid)����16����Ϊ��Ч��ID(u_gid) */
	void Getgid();

	/* ��ȡ��ǰ�û�����Ŀ¼ */
	void Pwd();

	/* ��鵱ǰ�û��Ƿ��ǳ����û� */
	bool SUser();

	void Cd(string dirName);
	void Mkdir(string dirName);
	void Create(string fileName, string mode);
	void Delete(string fileName);
	void Open(string fileName, string mode);
	void Close(string fd);
	void Seek(string fd, string offset, string origin);
	void Write(string fd, string inFile, string size);
	void Read(string fd, string outFile, string size);
	void Ls();

	bool IsError();
	void EchoError(enum ErrorCode err);
	int INodeMode(string mode);
	int FileMode(string mode);
	bool checkPathName(string path);
};

#endif // !USER_H

#ifndef FILE_H
#define FILE_H

#include "INode.h"

using namespace std;

/*
 * ���ļ����ƿ�File�ࡣ
 * �ýṹ��¼�˽��̴��ļ�
 * �Ķ���д�������ͣ��ļ���дλ�õȵȡ�
 */
class File
{
public:
	/* Enumerate */
	enum FileFlags
	{
		FREAD = 0x1,			/* ���������� */
		FWRITE = 0x2			/* д�������� */
	};

	/* Functions */
public:
	/* Constructors */
	File();
	/* Destructors */
	~File();


	/* Member */
	unsigned int f_flag;		/* �Դ��ļ��Ķ���д����Ҫ�� */
	int		f_count;			/* ��ǰ���ø��ļ����ƿ�Ľ������� */
	Inode* f_inode;			/* ָ����ļ����ڴ�Inodeָ�� */
	int		f_offset;			/* �ļ���дλ��ָ�� */
};


/*
 * ���̴��ļ���������(OpenFiles)�Ķ���
 * ���̵�u�ṹ�а���OpenFiles���һ������
 * ά���˵�ǰ���̵����д��ļ���
 */
class OpenFiles
{
	/* static members */
public:
	static const int NOFILES = 100;	/* ��������򿪵�����ļ��� */

	/* Functions */
public:
	/* Constructors */
	OpenFiles();
	/* Destructors */
	~OpenFiles();

	/*
	 * @comment ����������ļ�ʱ���ڴ��ļ����������з���һ�����б���
	 */
	int AllocFreeSlot();


	/*
	 * @comment �����û�ϵͳ�����ṩ���ļ�����������fd��
	 * �ҵ���Ӧ�Ĵ��ļ����ƿ�File�ṹ
	 */
	File* GetF(int fd);
	/*
	 * @comment Ϊ�ѷ��䵽�Ŀ���������fd���ѷ���Ĵ��ļ�����
	 * ����File������������ϵ
	 */
	void SetF(int fd, File* pFile);

	/* Members */
private:
	File* ProcessOpenFileTable[NOFILES];		/* File�����ָ�����飬ָ��ϵͳ���ļ����е�File���� */
};

/*
 * �ļ�I/O�Ĳ�����
 * ���ļ�����дʱ���õ��Ķ���дƫ������
 * �ֽ����Լ�Ŀ�������׵�ַ������
 */
class IOParameter
{
	/* Functions */
public:
	/* Constructors */
	IOParameter();
	/* Destructors */
	~IOParameter();

	/* Members */
public:
	unsigned char* m_Base;	/* ��ǰ����д�û�Ŀ��������׵�ַ */
	int m_Offset;	/* ��ǰ����д�ļ����ֽ�ƫ���� */
	int m_Count;	/* ��ǰ��ʣ��Ķ���д�ֽ����� */
};

#endif

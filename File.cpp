#include "common.h"

/*==============================class File===================================*/
File::File()
{
	this->f_count = 0;
	this->f_flag = 0;
	this->f_offset = 0;
	this->f_inode = NULL;
}

File::~File()
{
	//nothing to do here
}

/*==============================class OpenFiles===================================*/
OpenFiles::OpenFiles()
{
	memset(ProcessOpenFileTable, NULL, sizeof(ProcessOpenFileTable));
}

OpenFiles::~OpenFiles()
{
}

int OpenFiles::AllocFreeSlot()
{
	int i;
	User& u = globalUser;

	for (i = 0; i < OpenFiles::NOFILES; i++)
	{
		/* ���̴��ļ������������ҵ�������򷵻�֮ */
		if (this->ProcessOpenFileTable[i] == NULL)
		{
			/* ���ú���ջ�ֳ��������е�EAX�Ĵ�����ֵ����ϵͳ���÷���ֵ */
			u.u_ar0[User::EAX] = i;
			return i;
		}
	}

	u.u_ar0[User::EAX] = -1;   /* Open1����Ҫһ����־�������ļ��ṹ����ʧ��ʱ�����Ի���ϵͳ��Դ*/
	u.u_error = User::myEMFILE;
	return -1;
}


File* OpenFiles::GetF(int fd)
{
	File* pFile;
	User& u = globalUser;

	/* ������ļ���������ֵ�����˷�Χ */
	if (fd < 0 || fd >= OpenFiles::NOFILES)
	{
		u.u_error = User::myEBADF;
		return NULL;
	}

	pFile = this->ProcessOpenFileTable[fd];
	if (pFile == NULL)
	{
		u.u_error = User::myEBADF;
	}

	return pFile;	/* ��ʹpFile==NULLҲ���������ɵ���GetF�ĺ������жϷ���ֵ */
}

void OpenFiles::SetF(int fd, File* pFile)
{
	if (fd < 0 || fd >= OpenFiles::NOFILES)
	{
		return;
	}
	/* ���̴��ļ�������ָ��ϵͳ���ļ�������Ӧ��File�ṹ */
	this->ProcessOpenFileTable[fd] = pFile;
}

/*==============================class IOParameter===================================*/
IOParameter::IOParameter()
{
	//nothing to do here
}

IOParameter::~IOParameter()
{
	//nothing to do here
}


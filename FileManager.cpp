#include "common.h"

#include <iostream>
using namespace std;

FileManager::FileManager() {
    m_FileSystem = &globalFileSystem;
    m_OpenFileTable = &globalOpenFileTable;
    m_InodeTable = &globalINodeTable;
    rootDirInode = m_InodeTable->IGet(0);
    rootDirInode->i_count += 0xff;
}

FileManager::~FileManager() {

}

/*
* ���ܣ����ļ�
* Ч�����������ļ��ṹ���ڴ�i�ڵ㿪�� ��i_count Ϊ������i_count ++��
* */
void FileManager::Open() {
    User& u = globalUser;
    Inode* pINode;
    pINode = this->NameI(FileManager::OPEN);
    if (NULL == pINode) {
        return;
    }
    this->Open1(pINode, u.u_arg[1], 0);
}

void FileManager::Creat() {
    Inode* pINode;
    User& u = globalUser;
    unsigned int newACCMode = u.u_arg[1];

    /* ����Ŀ¼��ģʽΪ1����ʾ����������Ŀ¼����д�������� */
    pINode = this->NameI(FileManager::CREATE);

    /* û���ҵ���Ӧ��Inode����NameI���� */
    if (NULL == pINode) {
        if (u.u_error)
            return;
        pINode = this->MakNode(newACCMode);
        if (NULL == pINode)
            return;

        /* ������������ֲ����ڣ�ʹ�ò���trf = 2������open1()��*/
        this->Open1(pINode, File::FWRITE, 2);
        return;
    }

    /* ���NameI()�������Ѿ�����Ҫ�������ļ�������ո��ļ������㷨ITrunc()��*/
    this->Open1(pINode, File::FWRITE, 1);
    pINode->i_mode |= newACCMode;
}

/* ����NULL��ʾĿ¼����ʧ�ܣ�δ�ҵ�u.u_dirp��ָ��Ŀ¼ȫ·��
 * �����Ǹ�ָ�룬ָ���ļ����ڴ��i�ڵ�
 */
Inode* FileManager::NameI(enum DirectorySearchMode mode) {
    //if (NULL == rootDirINode) {
    //    rootDirINode = inodeTable->IGet(FileSystem::ROOT_INODE_NO);
    //    //rootDirINode���һ�Σ�ʼ�մ������ⱻIPut
    //    rootDirINode->i_count++;
    //    //open file table ?
    //    return rootDirINode;
    //}

    BufferManager& bufferManager = globalBufferManager;
    User& u = globalUser;
    Inode* pINode = u.u_cdir;
    Buf* pBuf;
    int freeEntryOffset;
    unsigned int index = 0, nindex = 0;

    if ('/' == u.u_dirp[0]) {
        nindex = ++index + 1;
        pINode = this->rootDirInode;
    }

    /* �����ͼ���ĺ�ɾ����ǰĿ¼�ļ������ */
    /*if ('\0' == curchar && mode != FileManager::OPEN)
    {
        u.u_error = User::ENOENT;
        goto out;
    }*/

    /* ���ѭ��ÿ�δ���pathname��һ��·������ */
    while (1) {
        if (u.u_error != User::myNOERROR) {
            break;
        }
        if (nindex >= u.u_dirp.length()) {
            return pINode;
        }

        /* ���Ҫ���������Ĳ���Ŀ¼�ļ����ͷ����Inode��Դ���˳� */
        if ((pINode->i_mode & Inode::IFMT) != Inode::IFDIR) {
            u.u_error = User::myENOTDIR;
            break;
        }

        nindex = u.u_dirp.find_first_of('/', index);
        memset(u.u_dbuf, 0, sizeof(u.u_dbuf));
        memcpy(u.u_dbuf, u.u_dirp.data() + index, (nindex == (unsigned int)string::npos ? u.u_dirp.length() : nindex) - index);
        index = nindex + 1;

        /* �ڲ�ѭ�����ֶ���u.u_dbuf[]�е�·���������������Ѱƥ���Ŀ¼�� */
        u.u_IOParam.m_Offset = 0;
        /* ����ΪĿ¼����� �����հ׵�Ŀ¼��*/
        u.u_IOParam.m_Count = pINode->i_size / sizeof(DirectoryEntry);
        freeEntryOffset = 0;
        pBuf = NULL;

        /* ��һ��Ŀ¼��Ѱ�� */
        while (1) {

            /* ��Ŀ¼���Ѿ�������� */
            if (0 == u.u_IOParam.m_Count) {
                if (NULL != pBuf) {
                    bufferManager.Brelse(pBuf);
                }

                /* ����Ǵ������ļ� */
                if (FileManager::CREATE == mode && nindex >= u.u_dirp.length()) {
                    u.u_pdir = pINode;
                    if (freeEntryOffset) {
                        u.u_IOParam.m_Offset = freeEntryOffset - sizeof(DirectoryEntry);
                    }
                    else {
                        pINode->i_flag |= Inode::IUPD;
                    }
                    return NULL;
                }
                u.u_error = User::myENOENT;
                goto out;
            }

            /* �Ѷ���Ŀ¼�ļ��ĵ�ǰ�̿飬��Ҫ������һĿ¼�������̿� */
            if (0 == u.u_IOParam.m_Offset % Inode::BLOCK_SIZE) {
                if (pBuf) {
                    bufferManager.Brelse(pBuf);
                }
                int phyBlkno = pINode->Bmap(u.u_IOParam.m_Offset / Inode::BLOCK_SIZE);
                pBuf = bufferManager.Bread(phyBlkno);
                //pBuffer->debug();
            }

            memcpy(&u.u_dent, pBuf->b_addr + (u.u_IOParam.m_Offset % Inode::BLOCK_SIZE), sizeof(u.u_dent));
            u.u_IOParam.m_Offset += sizeof(DirectoryEntry);
            u.u_IOParam.m_Count--;

            /* ����ǿ���Ŀ¼���¼����λ��Ŀ¼�ļ���ƫ���� */
            if (0 == u.u_dent.m_ino) {
                if (0 == freeEntryOffset) {
                    freeEntryOffset = u.u_IOParam.m_Offset;
                }
                continue;
            }

            if (!memcmp(u.u_dbuf, &u.u_dent.name, sizeof(DirectoryEntry) - 4)) {
                break;
            }
        }

        if (pBuf) {
            bufferManager.Brelse(pBuf);
        }

        /* �����ɾ���������򷵻ظ�Ŀ¼Inode����Ҫɾ���ļ���Inode����u.u_dent.m_ino�� */
        if (FileManager::DELETE == mode && nindex >= u.u_dirp.length()) {
            return pINode;
        }

        /*
        * ƥ��Ŀ¼��ɹ������ͷŵ�ǰĿ¼Inode������ƥ��ɹ���
        * Ŀ¼��m_ino�ֶλ�ȡ��Ӧ��һ��Ŀ¼���ļ���Inode��
        */
        this->m_InodeTable->IPut(pINode);
        pINode = this->m_InodeTable->IGet(u.u_dent.m_ino);

        if (NULL == pINode) {
            return NULL;
        }
    }

out:
    this->m_InodeTable->IPut(pINode);
    return NULL;
}

/*
* trf == 0��open����
* trf == 1��creat���ã�creat�ļ���ʱ��������ͬ�ļ������ļ�
* trf == 2��creat���ã�creat�ļ���ʱ��δ������ͬ�ļ������ļ��������ļ�����ʱ��һ������
* mode���������ļ�ģʽ����ʾ�ļ������� ����д���Ƕ�д
*/
void FileManager::Open1(Inode* pINode, int mode, int trf) {
    User& u = globalUser;

    /* ��creat�ļ���ʱ��������ͬ�ļ������ļ����ͷŸ��ļ���ռ�ݵ������̿� */
    if (1 == trf) {
        pINode->ITrunc();
    }

    /* ������ļ����ƿ�File�ṹ */
    File* pFile = this->m_OpenFileTable->FAlloc();
    if (NULL == pFile) {
        this->m_InodeTable->IPut(pINode);
        return;
    }

    /* ���ô��ļ���ʽ������File�ṹ���ڴ�Inode�Ĺ�����ϵ */
    pFile->f_flag = mode & (File::FREAD | File::FWRITE);
    pFile->f_inode = pINode;

    /* Ϊ�򿪻��ߴ����ļ��ĸ�����Դ���ѳɹ����䣬�������� */
    if (u.u_error == 0) {
        return;
    }
    else {  /* ����������ͷ���Դ */
        /* �ͷŴ��ļ������� */
        int fd = u.u_ar0[User::EAX];
        if (fd != -1) {
            u.u_ofiles.SetF(fd, NULL);
            /* �ݼ�File�ṹ��Inode�����ü��� ,File�ṹû���� f_countΪ0�����ͷ�File�ṹ��*/
            pFile->f_count--;
        }
        this->m_InodeTable->IPut(pINode);
    }
}

/* ��creat���á�
 * Ϊ�´������ļ�д�µ�i�ڵ�͸�Ŀ¼���µ�Ŀ¼��(��Ӧ������User�ṹ��)
 * ���ص�pINode�����������ڴ�i�ڵ㣬���е�i_count�� 1��
 */
Inode* FileManager::MakNode(unsigned int mode) {
    Inode* pINode;
    User& u = globalUser;

    /* ����һ������DiskInode������������ȫ����� */
    pINode = this->m_FileSystem->IAlloc();
    if (NULL == pINode) {
        return NULL;
    }

    pINode->i_flag |= (Inode::IACC | Inode::IUPD);
    pINode->i_mode = mode | Inode::IALLOC;
    pINode->i_nlink = 1;

    /* ��Ŀ¼��д��u.u_u_dent�����д��Ŀ¼�ļ� */
    this->WriteDir(pINode);
    return pINode;
}

/* ��creat���ӵ��á�
 * �������Լ���Ŀ¼��д����Ŀ¼���޸ĸ�Ŀ¼�ļ���i�ڵ� ������д�ش��̡�
 */
void FileManager::WriteDir(Inode* pINode) {
    User& u = globalUser;
    //cout << "i_number=" << pINode->i_number << endl;
    /* ����Ŀ¼����Inode��Ų��� */
    u.u_dent.m_ino = pINode->i_number;

    /* ����Ŀ¼����pathname�������� */
    memcpy(u.u_dent.name, u.u_dbuf, DirectoryEntry::DIRSIZ);

    u.u_IOParam.m_Count = DirectoryEntry::DIRSIZ + 4;
    u.u_IOParam.m_Base = (unsigned char*)&u.u_dent;

    /* ��Ŀ¼��д�븸Ŀ¼�ļ� */
    u.u_pdir->WriteI();
    this->m_InodeTable->IPut(u.u_pdir);
}

void FileManager::Close() {
    User& u = globalUser;
    int fd = u.u_arg[0];

    /* ��ȡ���ļ����ƿ�File�ṹ */
    File* pFile = u.u_ofiles.GetF(fd);
    if (NULL == pFile) {
        return;
    }

    /* �ͷŴ��ļ�������fd���ݼ�File�ṹ���ü��� */
    u.u_ofiles.SetF(fd, NULL);
    this->m_OpenFileTable->CloseF(pFile);
}

void FileManager::UnLink() {
    //ע��ɾ���ļ����д���й¶
    Inode* pINode;
    Inode* pDeleteINode;
    User& u = globalUser;

    pDeleteINode = this->NameI(FileManager::DELETE);
    if (NULL == pDeleteINode) {
        return;
    }

    pINode = this->m_InodeTable->IGet(u.u_dent.m_ino);
    if (NULL == pINode) {
        return;
    }

    /* д��������Ŀ¼�� */
    u.u_IOParam.m_Offset -= (DirectoryEntry::DIRSIZ + 4);
    u.u_IOParam.m_Base = (unsigned char*)&u.u_dent;
    u.u_IOParam.m_Count = DirectoryEntry::DIRSIZ + 4;

    u.u_dent.m_ino = 0;
    pDeleteINode->WriteI();

    /* �޸�inode�� */
    pINode->i_nlink--;
    pINode->i_flag |= Inode::IUPD;

    this->m_InodeTable->IPut(pDeleteINode);
    this->m_InodeTable->IPut(pINode);
}

void FileManager::Seek() {
    File* pFile;
    User& u = globalUser;
    int fd = u.u_arg[0];

    pFile = u.u_ofiles.GetF(fd);
    if (NULL == pFile) {
        return;  /* ��FILE�����ڣ�GetF��������� */
    }

    int m_Offset = u.u_arg[1];

    switch (u.u_arg[2]) {
        /* ��дλ������Ϊm_Offset */
        case 0:
            pFile->f_offset = m_Offset;
            break;
            /* ��дλ�ü�m_Offset(�����ɸ�) */
        case 1:
            pFile->f_offset += m_Offset;
            break;
            /* ��дλ�õ���Ϊ�ļ����ȼ�m_Offset */
        case 2:
            pFile->f_offset = pFile->f_inode->i_size + m_Offset;
            break;
        default:
            cout << " origin " << u.u_arg[2] << " is undefined ! \n";
            break;
    }
}

void FileManager::Read() {
    /* ֱ�ӵ���Rdwr()�������� */
    this->Rdwr(File::FREAD);
}

void FileManager::Write() {
    /* ֱ�ӵ���Rdwr()�������� */
    this->Rdwr(File::FWRITE);
}

void FileManager::Rdwr(enum File::FileFlags mode) {
    File* pFile;
    User& u = globalUser;

    /* ����Read()/Write()��ϵͳ���ò���fd��ȡ���ļ����ƿ�ṹ */
    pFile = u.u_ofiles.GetF(u.u_arg[0]);	/* fd */
    if (NULL == pFile) {
        /* �����ڸô��ļ���GetF�Ѿ����ù������룬�������ﲻ��Ҫ�������� */
        /*	u.u_error = User::EBADF;	*/
        return;
    }

    /* ��д��ģʽ����ȷ */
    if ((pFile->f_flag & mode) == 0) {
        u.u_error = User::myEACCES;
        return;
    }

    u.u_IOParam.m_Base = (unsigned char*)u.u_arg[1];     /* Ŀ�껺������ַ */
    u.u_IOParam.m_Count = u.u_arg[2];		/* Ҫ���/д���ֽ��� */

    u.u_IOParam.m_Offset = pFile->f_offset;   /* �����ļ���ʼ��λ�� */
    if (File::FREAD == mode) {
        pFile->f_inode->ReadI();
    }
    else {
        pFile->f_inode->WriteI();
    }

    /* ���ݶ�д�������ƶ��ļ���дƫ��ָ�� */
    pFile->f_offset += (u.u_arg[2] - u.u_IOParam.m_Count);

    /* ����ʵ�ʶ�д���ֽ������޸Ĵ��ϵͳ���÷���ֵ�ĺ���ջ��Ԫ */
    u.u_ar0[User::EAX] = u.u_arg[2] - u.u_IOParam.m_Count;
}

/* �ı䵱ǰ����Ŀ¼ */
void FileManager::ChDir() {
    Inode* pINode;
    User& u = globalUser;

    pINode = this->NameI(FileManager::OPEN);
    if (NULL == pINode) {
        return;
    }

    /* ���������ļ�����Ŀ¼�ļ� */
    if ((pINode->i_mode & Inode::IFMT) != Inode::IFDIR) {
        u.u_error = User::myENOTDIR;
        this->m_InodeTable->IPut(pINode);
        return;
    }

    u.u_cdir = pINode;
    /* ·�����ǴӸ�Ŀ¼'/'��ʼ����������u.u_curdir������ϵ�ǰ·������ */
    if (u.u_dirp[0] != '/') {
        u.u_curdir += u.u_dirp;
    }
    else {
        /* ����ǴӸ�Ŀ¼'/'��ʼ����ȡ��ԭ�й���Ŀ¼ */
        u.u_curdir = u.u_dirp;
    }
    if (u.u_curdir.back() != '/')
        u.u_curdir.push_back('/');
}


void FileManager::Ls() {
    User& u = globalUser;
    BufferManager& bufferManager = globalBufferManager;

    Inode* pINode = u.u_cdir;
    Buf* pBuffer = NULL;
    u.u_IOParam.m_Offset = 0;
    u.u_IOParam.m_Count = pINode->i_size / sizeof(DirectoryEntry);

    while (u.u_IOParam.m_Count) {
        if (0 == u.u_IOParam.m_Offset % Inode::BLOCK_SIZE) {
            if (pBuffer) {
                bufferManager.Brelse(pBuffer);
            }
            int phyBlkno = pINode->Bmap(u.u_IOParam.m_Offset / Inode::BLOCK_SIZE);
            pBuffer = bufferManager.Bread(phyBlkno);
        }
        memcpy(&u.u_dent, pBuffer->b_addr + (u.u_IOParam.m_Offset % Inode::BLOCK_SIZE), sizeof(u.u_dent));
        u.u_IOParam.m_Offset += sizeof(DirectoryEntry);
        u.u_IOParam.m_Count--;

        if (0 == u.u_dent.m_ino)
            continue;
        u.ls += u.u_dent.name;
        u.ls += "\n";
    }

    if (pBuffer) {
        bufferManager.Brelse(pBuffer);
    }
}
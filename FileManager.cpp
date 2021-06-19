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
* 功能：打开文件
* 效果：建立打开文件结构，内存i节点开锁 、i_count 为正数（i_count ++）
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

    /* 搜索目录的模式为1，表示创建；若父目录不可写，出错返回 */
    pINode = this->NameI(FileManager::CREATE);

    /* 没有找到相应的Inode，或NameI出错 */
    if (NULL == pINode) {
        if (u.u_error)
            return;
        pINode = this->MakNode(newACCMode);
        if (NULL == pINode)
            return;

        /* 如果创建的名字不存在，使用参数trf = 2来调用open1()。*/
        this->Open1(pINode, File::FWRITE, 2);
        return;
    }

    /* 如果NameI()搜索到已经存在要创建的文件，则清空该文件（用算法ITrunc()）*/
    this->Open1(pINode, File::FWRITE, 1);
    pINode->i_mode |= newACCMode;
}

/* 返回NULL表示目录搜索失败，未找到u.u_dirp中指定目录全路径
 * 否则是根指针，指向文件的内存打开i节点
 */
Inode* FileManager::NameI(enum DirectorySearchMode mode) {
    //if (NULL == rootDirINode) {
    //    rootDirINode = inodeTable->IGet(FileSystem::ROOT_INODE_NO);
    //    //rootDirINode多加一次，始终存在以免被IPut
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

    /* 如果试图更改和删除当前目录文件则出错 */
    /*if ('\0' == curchar && mode != FileManager::OPEN)
    {
        u.u_error = User::ENOENT;
        goto out;
    }*/

    /* 外层循环每次处理pathname中一段路径分量 */
    while (1) {
        if (u.u_error != User::myNOERROR) {
            break;
        }
        if (nindex >= u.u_dirp.length()) {
            return pINode;
        }

        /* 如果要进行搜索的不是目录文件，释放相关Inode资源则退出 */
        if ((pINode->i_mode & Inode::IFMT) != Inode::IFDIR) {
            u.u_error = User::myENOTDIR;
            break;
        }

        nindex = u.u_dirp.find_first_of('/', index);
        memset(u.u_dbuf, 0, sizeof(u.u_dbuf));
        memcpy(u.u_dbuf, u.u_dirp.data() + index, (nindex == (unsigned int)string::npos ? u.u_dirp.length() : nindex) - index);
        index = nindex + 1;

        /* 内层循环部分对于u.u_dbuf[]中的路径名分量，逐个搜寻匹配的目录项 */
        u.u_IOParam.m_Offset = 0;
        /* 设置为目录项个数 ，含空白的目录项*/
        u.u_IOParam.m_Count = pINode->i_size / sizeof(DirectoryEntry);
        freeEntryOffset = 0;
        pBuf = NULL;

        /* 在一个目录下寻找 */
        while (1) {

            /* 对目录项已经搜索完毕 */
            if (0 == u.u_IOParam.m_Count) {
                if (NULL != pBuf) {
                    bufferManager.Brelse(pBuf);
                }

                /* 如果是创建新文件 */
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

            /* 已读完目录文件的当前盘块，需要读入下一目录项数据盘块 */
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

            /* 如果是空闲目录项，记录该项位于目录文件中偏移量 */
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

        /* 如果是删除操作，则返回父目录Inode，而要删除文件的Inode号在u.u_dent.m_ino中 */
        if (FileManager::DELETE == mode && nindex >= u.u_dirp.length()) {
            return pINode;
        }

        /*
        * 匹配目录项成功，则释放当前目录Inode，根据匹配成功的
        * 目录项m_ino字段获取相应下一级目录或文件的Inode。
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
* trf == 0由open调用
* trf == 1由creat调用，creat文件的时候搜索到同文件名的文件
* trf == 2由creat调用，creat文件的时候未搜索到同文件名的文件，这是文件创建时更一般的情况
* mode参数：打开文件模式，表示文件操作是 读、写还是读写
*/
void FileManager::Open1(Inode* pINode, int mode, int trf) {
    User& u = globalUser;

    /* 在creat文件的时候搜索到同文件名的文件，释放该文件所占据的所有盘块 */
    if (1 == trf) {
        pINode->ITrunc();
    }

    /* 分配打开文件控制块File结构 */
    File* pFile = this->m_OpenFileTable->FAlloc();
    if (NULL == pFile) {
        this->m_InodeTable->IPut(pINode);
        return;
    }

    /* 设置打开文件方式，建立File结构和内存Inode的勾连关系 */
    pFile->f_flag = mode & (File::FREAD | File::FWRITE);
    pFile->f_inode = pINode;

    /* 为打开或者创建文件的各种资源都已成功分配，函数返回 */
    if (u.u_error == 0) {
        return;
    }
    else {  /* 如果出错则释放资源 */
        /* 释放打开文件描述符 */
        int fd = u.u_ar0[User::EAX];
        if (fd != -1) {
            u.u_ofiles.SetF(fd, NULL);
            /* 递减File结构和Inode的引用计数 ,File结构没有锁 f_count为0就是释放File结构了*/
            pFile->f_count--;
        }
        this->m_InodeTable->IPut(pINode);
    }
}

/* 由creat调用。
 * 为新创建的文件写新的i节点和父目录中新的目录项(相应参数在User结构中)
 * 返回的pINode是上了锁的内存i节点，其中的i_count是 1。
 */
Inode* FileManager::MakNode(unsigned int mode) {
    Inode* pINode;
    User& u = globalUser;

    /* 分配一个空闲DiskInode，里面内容已全部清空 */
    pINode = this->m_FileSystem->IAlloc();
    if (NULL == pINode) {
        return NULL;
    }

    pINode->i_flag |= (Inode::IACC | Inode::IUPD);
    pINode->i_mode = mode | Inode::IALLOC;
    pINode->i_nlink = 1;

    /* 将目录项写入u.u_u_dent，随后写入目录文件 */
    this->WriteDir(pINode);
    return pINode;
}

/* 由creat子子调用。
 * 把属于自己的目录项写进父目录，修改父目录文件的i节点 、将其写回磁盘。
 */
void FileManager::WriteDir(Inode* pINode) {
    User& u = globalUser;
    //cout << "i_number=" << pINode->i_number << endl;
    /* 设置目录项中Inode编号部分 */
    u.u_dent.m_ino = pINode->i_number;

    /* 设置目录项中pathname分量部分 */
    memcpy(u.u_dent.name, u.u_dbuf, DirectoryEntry::DIRSIZ);

    u.u_IOParam.m_Count = DirectoryEntry::DIRSIZ + 4;
    u.u_IOParam.m_Base = (unsigned char*)&u.u_dent;

    /* 将目录项写入父目录文件 */
    u.u_pdir->WriteI();
    this->m_InodeTable->IPut(u.u_pdir);
}

void FileManager::Close() {
    User& u = globalUser;
    int fd = u.u_arg[0];

    /* 获取打开文件控制块File结构 */
    File* pFile = u.u_ofiles.GetF(fd);
    if (NULL == pFile) {
        return;
    }

    /* 释放打开文件描述符fd，递减File结构引用计数 */
    u.u_ofiles.SetF(fd, NULL);
    this->m_OpenFileTable->CloseF(pFile);
}

void FileManager::UnLink() {
    //注意删除文件夹有磁盘泄露
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

    /* 写入清零后的目录项 */
    u.u_IOParam.m_Offset -= (DirectoryEntry::DIRSIZ + 4);
    u.u_IOParam.m_Base = (unsigned char*)&u.u_dent;
    u.u_IOParam.m_Count = DirectoryEntry::DIRSIZ + 4;

    u.u_dent.m_ino = 0;
    pDeleteINode->WriteI();

    /* 修改inode项 */
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
        return;  /* 若FILE不存在，GetF有设出错码 */
    }

    int m_Offset = u.u_arg[1];

    switch (u.u_arg[2]) {
        /* 读写位置设置为m_Offset */
        case 0:
            pFile->f_offset = m_Offset;
            break;
            /* 读写位置加m_Offset(可正可负) */
        case 1:
            pFile->f_offset += m_Offset;
            break;
            /* 读写位置调整为文件长度加m_Offset */
        case 2:
            pFile->f_offset = pFile->f_inode->i_size + m_Offset;
            break;
        default:
            cout << " origin " << u.u_arg[2] << " is undefined ! \n";
            break;
    }
}

void FileManager::Read() {
    /* 直接调用Rdwr()函数即可 */
    this->Rdwr(File::FREAD);
}

void FileManager::Write() {
    /* 直接调用Rdwr()函数即可 */
    this->Rdwr(File::FWRITE);
}

void FileManager::Rdwr(enum File::FileFlags mode) {
    File* pFile;
    User& u = globalUser;

    /* 根据Read()/Write()的系统调用参数fd获取打开文件控制块结构 */
    pFile = u.u_ofiles.GetF(u.u_arg[0]);	/* fd */
    if (NULL == pFile) {
        /* 不存在该打开文件，GetF已经设置过出错码，所以这里不需要再设置了 */
        /*	u.u_error = User::EBADF;	*/
        return;
    }

    /* 读写的模式不正确 */
    if ((pFile->f_flag & mode) == 0) {
        u.u_error = User::myEACCES;
        return;
    }

    u.u_IOParam.m_Base = (unsigned char*)u.u_arg[1];     /* 目标缓冲区首址 */
    u.u_IOParam.m_Count = u.u_arg[2];		/* 要求读/写的字节数 */

    u.u_IOParam.m_Offset = pFile->f_offset;   /* 设置文件起始读位置 */
    if (File::FREAD == mode) {
        pFile->f_inode->ReadI();
    }
    else {
        pFile->f_inode->WriteI();
    }

    /* 根据读写字数，移动文件读写偏移指针 */
    pFile->f_offset += (u.u_arg[2] - u.u_IOParam.m_Count);

    /* 返回实际读写的字节数，修改存放系统调用返回值的核心栈单元 */
    u.u_ar0[User::EAX] = u.u_arg[2] - u.u_IOParam.m_Count;
}

/* 改变当前工作目录 */
void FileManager::ChDir() {
    Inode* pINode;
    User& u = globalUser;

    pINode = this->NameI(FileManager::OPEN);
    if (NULL == pINode) {
        return;
    }

    /* 搜索到的文件不是目录文件 */
    if ((pINode->i_mode & Inode::IFMT) != Inode::IFDIR) {
        u.u_error = User::myENOTDIR;
        this->m_InodeTable->IPut(pINode);
        return;
    }

    u.u_cdir = pINode;
    /* 路径不是从根目录'/'开始，则在现有u.u_curdir后面加上当前路径分量 */
    if (u.u_dirp[0] != '/') {
        u.u_curdir += u.u_dirp;
    }
    else {
        /* 如果是从根目录'/'开始，则取代原有工作目录 */
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
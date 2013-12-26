// Error macros for TinyFS

// Check if nBytes are enough to create a TFS.
#define ESIZE -1
// openDisk fails
#define EOPENDISK -2
// readBlock fails
#define EREADBLOCK -3
// bad block code
#define EBADBLOCK -4
// bad magic number
#define EBADMAGNUM -5
// bad super block
#define ENOSUPER -6
// TinyFS is mounted
#define EMOUNTED -7
// TinyFS has not been mounted
#define EMOUNT -8
// filename length exceeds max file name length
#define EFILELEN -9
// filedescriptor is not valid
#define EINVALIDFD -10
// filename does not exist
#define EMISSFILE -11
// table is not empty, unable to unmount
#define ETABLE -12
// file is set to read only
#define EREADONLY -13
// file pointer is past the end of file
#define EPEOF -14
// offset is less than 0
#define EOFFSET -15
// no more free blocks
#define ENOFREEBLK -16
// no such file or directory
#define ENOENT -17
// directory is not empty
#define ENOEMPTY -18
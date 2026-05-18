#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#define BLOCK_SIZE          4096
#define BOOT_SECTOR_SIZE    512
#define MAGIC               0x4D494E46
#define MAX_FILENAME        255
#define MAX_PATH            1024
#define MAX_OPEN_FILES      64
#define DIRECT_BLOCKS       12
#define INDIRECT_BLOCK      1
#define TOTAL_BLOCK_PTRS    (DIRECT_BLOCKS + INDIRECT_BLOCK)


// ============== 磁盘数据结构 ==============
struct boot_sector {
    uint8_t  boot_code[446];
    uint16_t sector_size;             // 512
    uint8_t  sectors_per_cluster;     // 8 (因为BLOCK_SIZE=4096)
    uint16_t reserved_sectors;        // 1
    uint8_t  num_fats;                // 0
    uint16_t max_root_entries;        // 0 (不使用)
    uint16_t total_sectors_16;
    uint8_t  media_descriptor;        // 0xF8
    uint16_t sectors_per_fat;         // 0
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint8_t  drive_number;
    uint8_t  reserved;
    uint8_t  boot_signature;       // 0x29
    uint32_t volume_id;
    char     volume_label[11];
    char     fs_type[8];           // "MINFS"
    uint8_t  boot_code_rest[0];
    uint16_t signature;            // 0xAA55
} __attribute__((packed));
struct superblock {
    uint32_t magic;
    uint32_t block_size;
    uint32_t total_blocks;
    uint32_t inode_table_blocks;
    uint32_t inode_bitmap_block;
    uint32_t data_bitmap_block;
    uint32_t inode_table_start;
    uint32_t data_start;
    uint32_t total_inodes;
    uint32_t free_inodes;
    uint32_t free_blocks;
    uint32_t root_inode;
    uint32_t inode_bitmap_blocks;
    uint32_t data_bitmap_blocks;
    uint32_t inode_bitmap_start;
};
struct inode {
    uint32_t mode;
    uint32_t size;
    uint32_t blocks;
    uint32_t direct[DIRECT_BLOCKS];
    uint32_t indirect;
    uint32_t atime;
    uint32_t mtime;
    uint32_t ctime;
    uint32_t nlink;
    char     filename[MAX_FILENAME];
};
struct dir_entry { uint32_t inode_no; char filename[MAX_FILENAME]; };
struct indirect_block { uint32_t blocks[BLOCK_SIZE / sizeof(uint32_t)]; };


////////////////////////////////////
struct filesystem {
    int             fd;
    int             free_inodes;
    uint32_t        block_size;
    uint32_t        total_blocks;
    uint32_t        inode_table_blocks;
    uint32_t        inode_bitmap_block;
    uint32_t        data_bitmap_block;
    uint32_t        inode_table_start;
    uint32_t        data_start;
    uint32_t        total_inodes;
    uint32_t        root_inode;
};
struct open_file {
    uint32_t        inode_no;
    uint32_t        offset;
    struct inode    inode;
    int             valid;
};
static struct filesystem fs;
static struct open_file open_files[MAX_OPEN_FILES];
void read_block(uint32_t block_no, void *buf) { lseek(fs.fd, block_no * fs.block_size, SEEK_SET); read(fs.fd, buf, fs.block_size); }
void write_block(uint32_t block_no, void *buf) { lseek(fs.fd, block_no * fs.block_size, SEEK_SET); write(fs.fd, buf, fs.block_size); }
void set_bit(uint8_t *bitmap, uint32_t bit) { bitmap[bit / 8] |= (1 << (bit % 8)); }
void clear_bit(uint8_t *bitmap, uint32_t bit) { bitmap[bit / 8] &= ~(1 << (bit % 8)); }
int test_bit(uint8_t *bitmap, uint32_t bit) { return (bitmap[bit / 8] >> (bit % 8)) & 1; }
uint32_t allocate_inode(void) {
    uint32_t blocks_for_bitmap = (fs.total_inodes + BLOCK_SIZE * 8 - 1) / (BLOCK_SIZE * 8);
    uint8_t *bitmap = malloc(BLOCK_SIZE);
    for (uint32_t i = 0; i < blocks_for_bitmap; i++) {
        read_block(fs.inode_bitmap_block + i, bitmap);
        for (uint32_t j = 0; j < BLOCK_SIZE * 8 && i * BLOCK_SIZE * 8 + j < fs.total_inodes; j++) {
            if (!test_bit(bitmap, j)) {
                set_bit(bitmap, j);
                write_block(fs.inode_bitmap_block + i, bitmap);
                free(bitmap);
                struct superblock sb;
                lseek(fs.fd, BLOCK_SIZE, SEEK_SET);
                read(fs.fd, &sb, sizeof(sb));
                sb.free_inodes--;
                lseek(fs.fd, BLOCK_SIZE, SEEK_SET);
                write(fs.fd, &sb, sizeof(sb));
                fs.free_inodes = sb.free_inodes;
                return i * BLOCK_SIZE * 8 + j;
            }
        }
    }
    free(bitmap); return 0xFFFFFFFF;
}
void free_inode(uint32_t inode_no) {
    uint32_t block_idx = inode_no / (BLOCK_SIZE * 8);
    uint32_t bit_idx = inode_no % (BLOCK_SIZE * 8);
    uint8_t *bitmap = malloc(BLOCK_SIZE);
    read_block(fs.inode_bitmap_block + block_idx, bitmap);
    clear_bit(bitmap, bit_idx);
    write_block(fs.inode_bitmap_block + block_idx, bitmap);
    free(bitmap);
}
uint32_t allocate_block(void) {
    uint32_t total_blocks = fs.total_blocks - fs.data_start;
    uint32_t blocks_for_bitmap = (total_blocks + BLOCK_SIZE * 8 - 1) / (BLOCK_SIZE * 8);
    uint8_t *bitmap = malloc(BLOCK_SIZE);
    for (uint32_t i = 0; i < blocks_for_bitmap; i++) {
        read_block(fs.data_bitmap_block + i, bitmap);
        for (uint32_t j = 0; j < BLOCK_SIZE * 8 && i * BLOCK_SIZE * 8 + j < total_blocks; j++) {
            if (!test_bit(bitmap, j)) {
                set_bit(bitmap, j);
                write_block(fs.data_bitmap_block + i, bitmap);
                free(bitmap);
                return fs.data_start + i * BLOCK_SIZE * 8 + j;
            }
        }
    }
    free(bitmap); return 0xFFFFFFFF;
}
void free_block(uint32_t block_no) {
    uint32_t total_blocks = fs.total_blocks - fs.data_start;
    uint32_t logical_block = block_no - fs.data_start;
    uint32_t block_idx = logical_block / (BLOCK_SIZE * 8);
    uint32_t bit_idx = logical_block % (BLOCK_SIZE * 8);
    uint8_t *bitmap = malloc(BLOCK_SIZE);
    read_block(fs.data_bitmap_block + block_idx, bitmap);
    clear_bit(bitmap, bit_idx);
    write_block(fs.data_bitmap_block + block_idx, bitmap);
    free(bitmap);
}
void read_inode(uint32_t inode_no, struct inode *inode) {
    uint32_t block_idx = inode_no / (BLOCK_SIZE / sizeof(struct inode));
    uint32_t offset_in_block = inode_no % (BLOCK_SIZE / sizeof(struct inode));
    uint8_t *block_buf = malloc(BLOCK_SIZE);
    read_block(fs.inode_table_start + block_idx, block_buf);
    memcpy(inode, block_buf + offset_in_block * sizeof(struct inode), sizeof(struct inode));
    free(block_buf);
}
void write_inode(uint32_t inode_no, struct inode *inode) {
    uint32_t block_idx = inode_no / (BLOCK_SIZE / sizeof(struct inode));
    uint32_t offset_in_block = inode_no % (BLOCK_SIZE / sizeof(struct inode));
    uint8_t *block_buf = malloc(BLOCK_SIZE);
    read_block(fs.inode_table_start + block_idx, block_buf);
    memcpy(block_buf + offset_in_block * sizeof(struct inode), inode, sizeof(struct inode));
    write_block(fs.inode_table_start + block_idx, block_buf);
    free(block_buf);
}
int walk_path(const char *path, uint32_t *inode_no) {
    if (!path || !inode_no) return -1;
    while (*path == '/') path++;
    if (strlen(path) == 0) { *inode_no = fs.root_inode; return 0; }
    uint32_t current_inode = fs.root_inode;
    char component[MAX_FILENAME];
    while (*path) {
        int i = 0;
        while (*path && *path != '/' && i < MAX_FILENAME - 1) { component[i++] = *path++; }
        component[i] = '\0';
        while (*path == '/') path++;
        struct inode dir_inode;
        read_inode(current_inode, &dir_inode);
        if ((dir_inode.mode & 0170000) != 040000) { return -1; }
        uint32_t total_entries = dir_inode.size / sizeof(struct dir_entry);
        int found = -1;
        for (uint32_t j = 0; j < dir_inode.blocks && j < DIRECT_BLOCKS; j++) {
            if (dir_inode.direct[j] == 0) continue;
            uint8_t *block = malloc(BLOCK_SIZE);
            read_block(dir_inode.direct[j], block);
            uint32_t entries_in_block = BLOCK_SIZE / sizeof(struct dir_entry);
            uint32_t max_entries = (j == dir_inode.blocks - 1) ? (total_entries % entries_in_block) : entries_in_block;
            if (max_entries == 0 && j == dir_inode.blocks - 1) max_entries = entries_in_block;
            struct dir_entry *entries = (struct dir_entry*)block;
            for (uint32_t k = 0; k < max_entries && (j * entries_in_block + k) < total_entries; k++) {
                if (strcmp(entries[k].filename, component) == 0) { current_inode = entries[k].inode_no; found = 1; break; }
            }
            free(block);
            if (found == 1) break;
        }
        if (found == -1) return -1;
    }
    *inode_no = current_inode;
    return 0;
}
int find_file(const char *path, uint32_t *inode_no) { return walk_path(path, inode_no); }




////////////////////////////////////
int fs_format(const char *filename, uint32_t num_blocks) {
    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return -1;
    lseek(fd, num_blocks * BLOCK_SIZE - 1, SEEK_SET);
    write(fd, "", 1);
    uint32_t total_blocks = num_blocks;
    uint32_t inode_count = total_blocks / 4;
    if (inode_count < 10) inode_count = 10;
    uint32_t inode_table_blocks = (inode_count * sizeof(struct inode) + BLOCK_SIZE - 1) / BLOCK_SIZE;
    uint32_t inode_bitmap_blocks = (inode_count + BLOCK_SIZE * 8 - 1) / (BLOCK_SIZE * 8);
    uint32_t data_blocks = total_blocks - 1 - 1 - inode_bitmap_blocks - 1 - inode_table_blocks;
    uint32_t data_bitmap_blocks = (data_blocks + BLOCK_SIZE * 8 - 1) / (BLOCK_SIZE * 8);
    uint32_t current_block = 0;
    struct boot_sector boot;
    memset(&boot, 0, sizeof(boot));
    boot.sector_size = 512;
    boot.sectors_per_cluster = 8;
    boot.reserved_sectors = 1;
    boot.num_fats = 0;
    boot.media_descriptor = 0xF8;
    boot.boot_signature = 0x29;
    boot.volume_id = 0x12345678;
    strcpy(boot.volume_label, "MINFS     ");
    strcpy(boot.fs_type, "MINFS");
    boot.signature = 0xAA55;
    lseek(fd, 0, SEEK_SET);
    write(fd, &boot, sizeof(boot));
    lseek(fd, BLOCK_SIZE - 1, SEEK_SET);
    write(fd, "", 1);
    current_block++;
    struct superblock sb;
    memset(&sb, 0, sizeof(sb));
    sb.magic = MAGIC;
    sb.block_size = BLOCK_SIZE;
    sb.total_blocks = total_blocks;
    sb.inode_table_blocks = inode_table_blocks;
    sb.inode_bitmap_block = current_block + 1;
    sb.data_bitmap_block = sb.inode_bitmap_block + inode_bitmap_blocks;
    sb.inode_table_start = sb.data_bitmap_block + data_bitmap_blocks;
    sb.data_start = sb.inode_table_start + inode_table_blocks;
    sb.total_inodes = inode_count;
    sb.free_inodes = inode_count;
    sb.free_blocks = data_blocks;
    sb.root_inode = 0;
    lseek(fd, current_block * BLOCK_SIZE, SEEK_SET);
    write(fd, &sb, sizeof(sb));
    current_block++;
    uint8_t *zero_bitmap = calloc(BLOCK_SIZE, 1);
    for (uint32_t i = 0; i < inode_bitmap_blocks; i++) {
        lseek(fd, (sb.inode_bitmap_block + i) * BLOCK_SIZE, SEEK_SET);
        if (i == 0) {
            uint8_t *first_bitmap = malloc(BLOCK_SIZE);
            memset(first_bitmap, 0, BLOCK_SIZE);
            set_bit(first_bitmap, 0);
            write(fd, first_bitmap, BLOCK_SIZE);
            free(first_bitmap);
        } else { write(fd, zero_bitmap, BLOCK_SIZE); }
    }
    for (uint32_t i = 0; i < data_bitmap_blocks; i++) { lseek(fd, (sb.data_bitmap_block + i) * BLOCK_SIZE, SEEK_SET); write(fd, zero_bitmap, BLOCK_SIZE); }
    free(zero_bitmap);
    uint8_t *zero_inode = calloc(BLOCK_SIZE, 1);
    for (uint32_t i = 0; i < inode_table_blocks; i++) { lseek(fd, (sb.inode_table_start + i) * BLOCK_SIZE, SEEK_SET); write(fd, zero_inode, BLOCK_SIZE); }
    free(zero_inode);
    struct inode root_inode;
    memset(&root_inode, 0, sizeof(root_inode));
    root_inode.mode = 040755;
    root_inode.size = 0;
    root_inode.blocks = 0;
    root_inode.nlink = 1;
    strcpy(root_inode.filename, "/");
    lseek(fd, sb.inode_table_start * BLOCK_SIZE, SEEK_SET);
    write(fd, &root_inode, sizeof(root_inode));
    close(fd);
    return 0;
}
int fs_mount(const char *filename) {
    fs.fd = open(filename, O_RDWR);
    if (fs.fd < 0) return -1;
    struct superblock sb;
    lseek(fs.fd, BLOCK_SIZE, SEEK_SET);
    read(fs.fd, &sb, sizeof(sb));
    if (sb.magic != MAGIC) { close(fs.fd); return -1; }
    fs.block_size = sb.block_size;
    fs.total_blocks = sb.total_blocks;
    fs.inode_table_blocks = sb.inode_table_blocks;
    fs.inode_bitmap_block = sb.inode_bitmap_block;
    fs.data_bitmap_block = sb.data_bitmap_block;
    fs.inode_table_start = sb.inode_table_start;
    fs.data_start = sb.data_start;
    fs.total_inodes = sb.total_inodes;
    fs.root_inode = sb.root_inode;
    for (int i = 0; i < MAX_OPEN_FILES; i++) { open_files[i].valid = 0; }
    return 0;
}
int fs_umount(void) {
    struct superblock sb;
    lseek(fs.fd, BLOCK_SIZE, SEEK_SET);
    read(fs.fd, &sb, sizeof(sb));
    sb.free_inodes = sb.total_inodes;
    sb.free_blocks = sb.total_blocks;
    for (int i = 0; i < sb.inode_bitmap_blocks; i++) {
        unsigned char bitmap[BLOCK_SIZE];
        lseek(fs.fd, sb.inode_bitmap_start + i * BLOCK_SIZE, SEEK_SET);
        read(fs.fd, bitmap, BLOCK_SIZE);
        for (int j = 0; j < BLOCK_SIZE * 8; j++) {
            if (bitmap[j / 8] & (1 << (j % 8))) sb.free_inodes--;
        }
    }
    for (int i = 0; i < sb.data_bitmap_blocks; i++) {
        unsigned char bitmap[BLOCK_SIZE];
        lseek(fs.fd, sb.data_bitmap_block + i * BLOCK_SIZE, SEEK_SET);
        read(fs.fd, bitmap, BLOCK_SIZE);
        for (int j = 0; j < BLOCK_SIZE * 8; j++) {
            if (bitmap[j / 8] & (1 << (j % 8))) sb.free_blocks--;
        }
    }
    lseek(fs.fd, BLOCK_SIZE, SEEK_SET);
    write(fs.fd, &sb, sizeof(sb));
    close(fs.fd);
    return 0;
}
int fs_create(const char *path) {
    uint32_t inode_no;
    if (find_file(path, &inode_no) == 0) { return -1; }
    inode_no = allocate_inode();
    if (inode_no == 0xFFFFFFFF) return -1;
    struct inode inode;
    memset(&inode, 0, sizeof(inode));
    inode.mode = 0100644;
    inode.size = 0;
    inode.blocks = 0;
    inode.nlink = 1;
    const char *filename = path;
    if (strrchr(path, '/')) { filename = strrchr(path, '/') + 1; }
    strncpy(inode.filename, filename, MAX_FILENAME - 1);
    write_inode(inode_no, &inode);
    struct inode root_inode;
    read_inode(fs.root_inode, &root_inode);
    struct dir_entry entry;
    entry.inode_no = inode_no;
    strcpy(entry.filename, filename);
    uint32_t block_no = 0;
    if (root_inode.blocks == 0) {
        block_no = allocate_block();
        if (block_no == 0xFFFFFFFF) { free_inode(inode_no); return -1; }
        root_inode.direct[0] = block_no;
        root_inode.blocks = 1;
    } else { block_no = root_inode.direct[root_inode.blocks - 1]; }
    uint8_t *block_buf = malloc(BLOCK_SIZE);
    read_block(block_no, block_buf);
    uint32_t entry_offset = root_inode.size;
    memcpy(block_buf + entry_offset, &entry, sizeof(entry));
    write_block(block_no, block_buf);
    root_inode.size += sizeof(entry);
    write_inode(fs.root_inode, &root_inode);
    free(block_buf);
    return 0;
}
int fs_open(const char *path) {
    uint32_t inode_no;
    if (find_file(path, &inode_no) != 0) return -1;
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (!open_files[i].valid) {
            open_files[i].valid = 1;
            open_files[i].inode_no = inode_no;
            open_files[i].offset = 0;
            read_inode(inode_no, &open_files[i].inode);
            return i;
        }
    } return -1;
}
int fs_read(int fd, void *buf, uint32_t size) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !open_files[fd].valid) return -1;
    struct open_file *of = &open_files[fd];
    struct inode *inode = &of->inode;
    if (of->offset >= inode->size) return 0;
    uint32_t to_read = size;
    if (of->offset + to_read > inode->size) { to_read = inode->size - of->offset; }
    uint32_t bytes_read = 0;
    uint8_t *buf_ptr = (uint8_t *)buf;
    while (bytes_read < to_read) {
        uint32_t block_idx = (of->offset + bytes_read) / BLOCK_SIZE;
        uint32_t offset_in_block = (of->offset + bytes_read) % BLOCK_SIZE;
        uint32_t block_no = 0;
        if (block_idx < DIRECT_BLOCKS) { block_no = inode->direct[block_idx];
        } else if (block_idx < DIRECT_BLOCKS + BLOCK_SIZE / sizeof(uint32_t)) {
            if (inode->indirect == 0) break;
            uint32_t indirect_blocks[BLOCK_SIZE / sizeof(uint32_t)];
            read_block(inode->indirect, indirect_blocks);
            block_no = indirect_blocks[block_idx - DIRECT_BLOCKS];
        } else { break; }
        if (block_no == 0) break;
        uint8_t *block_buf = malloc(BLOCK_SIZE);
        read_block(block_no, block_buf);
        uint32_t chunk = BLOCK_SIZE - offset_in_block;
        if (chunk > to_read - bytes_read) { chunk = to_read - bytes_read; }
        memcpy(buf_ptr + bytes_read, block_buf + offset_in_block, chunk);
        bytes_read += chunk;
        free(block_buf);
    }
    of->offset += bytes_read;
    return bytes_read;
}
int fs_write(int fd, void *buf, uint32_t size) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !open_files[fd].valid) return -1;
    struct open_file *of = &open_files[fd];
    struct inode *inode = &of->inode;
    uint32_t bytes_written = 0;
    uint8_t *buf_ptr = (uint8_t *)buf;
    uint32_t block_idx = (of->offset + bytes_written) / BLOCK_SIZE;
    uint32_t offset_in_block = (of->offset + bytes_written) % BLOCK_SIZE;
    uint32_t block_no = 0;
    while (bytes_written < size) {
        if (block_idx < DIRECT_BLOCKS) { block_no = inode->direct[block_idx];
        if (block_no == 0) { block_no = allocate_block(); if (block_no == 0xFFFFFFFF) break; inode->direct[block_idx] = block_no; inode->blocks++; }
        } else if (block_idx < DIRECT_BLOCKS + BLOCK_SIZE / sizeof(uint32_t)) {
        if (inode->indirect == 0) { inode->indirect = allocate_block(); if (inode->indirect == 0xFFFFFFFF) break; inode->blocks++; uint8_t *indirect_buf = calloc(BLOCK_SIZE, 1); write_block(inode->indirect, indirect_buf); free(indirect_buf); }
        uint32_t indirect_blocks[BLOCK_SIZE / sizeof(uint32_t)];
        read_block(inode->indirect, indirect_blocks);
        block_no = indirect_blocks[block_idx - DIRECT_BLOCKS];
        if (block_no == 0) {
            block_no = allocate_block();
            if (block_no == 0xFFFFFFFF) break;
            indirect_blocks[block_idx - DIRECT_BLOCKS] = block_no;
            write_block(inode->indirect, indirect_blocks);
            inode->blocks++;
        }
    } else {
        break;
    }
    }
    uint8_t *block_buf = malloc(BLOCK_SIZE);
    read_block(block_no, block_buf);
    uint32_t chunk = BLOCK_SIZE - offset_in_block;
    if (chunk > size - bytes_written) { chunk = size - bytes_written; }
    memcpy(block_buf + offset_in_block, buf_ptr + bytes_written, chunk);
    write_block(block_no, block_buf);
    bytes_written += chunk;
    free(block_buf);
    if (of->offset + bytes_written > inode->size) { inode->size = of->offset + bytes_written; }
    of->offset += bytes_written;
    write_inode(of->inode_no, inode);
    return bytes_written;
}
void fs_close(int fd) {
    if (fd >= 0 && fd < MAX_OPEN_FILES && open_files[fd].valid) { open_files[fd].valid = 0; }
}
int fs_unlink(const char *path) {
    uint32_t inode_no;
    if (find_file(path, &inode_no) != 0) return -1;
    if (inode_no == fs.root_inode) return -1;
    struct inode root_inode;
    read_inode(fs.root_inode, &root_inode);
    uint32_t total_entries = root_inode.size / sizeof(struct dir_entry);
    struct dir_entry *entries = malloc(root_inode.size);
    if (!entries) return -1;
    uint32_t bytes_read = 0;
    for (uint32_t i = 0; i < root_inode.blocks; i++) {
        uint8_t *block = malloc(BLOCK_SIZE);
        if (!block) { free(entries); return -1; }
        uint32_t block_no = root_inode.direct[i];
        read_block(block_no, block);
        uint32_t to_copy = (root_inode.size - bytes_read) < BLOCK_SIZE ? (root_inode.size - bytes_read) : BLOCK_SIZE;
        memcpy((uint8_t*)entries + bytes_read, block, to_copy);
        bytes_read += to_copy;
        free(block);
    }
    const char *filename = strrchr(path, '/');
    if (filename) filename++; else filename = path;
    int found = -1;
    for (uint32_t i = 0; i < total_entries; i++) {
        if (strcmp(entries[i].filename, filename) == 0) { found = i; break; }
    }
    if (found < 0) { free(entries); return -1;
    }
    for (uint32_t i = found; i < total_entries - 1; i++) { entries[i] = entries[i + 1]; }
    root_inode.size -= sizeof(struct dir_entry);
    bytes_read = 0;
    for (uint32_t i = 0; i < root_inode.blocks; i++) {
        uint8_t *block = malloc(BLOCK_SIZE);
        if (!block) { free(entries); return -1; }
        uint32_t block_no = root_inode.direct[i];
        uint32_t to_copy = (root_inode.size - bytes_read) < BLOCK_SIZE ? (root_inode.size - bytes_read) : BLOCK_SIZE;
        if (to_copy > 0) { memcpy(block, (uint8_t*)entries + bytes_read, to_copy); write_block(block_no, block); }
        bytes_read += to_copy;
        free(block);
    }
    free(entries);
    write_inode(fs.root_inode, &root_inode);
    struct inode inode;
    read_inode(inode_no, &inode);
    for (int i = 0; i < DIRECT_BLOCKS && inode.direct[i] != 0; i++) { free_block(inode.direct[i]); }
    if (inode.indirect != 0) {
        uint32_t indirect_blocks[BLOCK_SIZE / sizeof(uint32_t)];
        read_block(inode.indirect, indirect_blocks);
        for (int i = 0; i < BLOCK_SIZE / sizeof(uint32_t) && indirect_blocks[i] != 0; i++) { free_block(indirect_blocks[i]); }
        free_block(inode.indirect);
    }
    free_inode(inode_no);
    return 0;
}


// AI写的测试程序，但是我这边好像没有成功创建img，你看看是哪里有问题
int main(int argc, char *argv[]) {
    const char *disk_file = "minfs.img";
    uint32_t num_blocks = 256;  // 1MB (256 * 4KB = 1MB)
    printf("创建 1MB 文件系统镜像...\n");
    if (fs_format(disk_file, num_blocks) != 0) {
        printf("格式化失败\n");
        return -1;
    }
    
    printf("成功创建 %s (大小: %u 字节)\n", disk_file, num_blocks * BLOCK_SIZE);
    return 0;
}
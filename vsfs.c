#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_SIZE 4096
#define TOTAL_BLOCKS 64
#define INODE_SIZE 256
#define INODES_PER_BLOCK (BLOCK_SIZE / INODE_SIZE)
#define INODE_TABLE_BLOCKS 5
#define INODE_TABLE_START 3
#define DATA_BLOCK_START 8
#define MAX_INODES (INODE_TABLE_BLOCKS * INODES_PER_BLOCK)
#define DATA_BLOCK_COUNT (TOTAL_BLOCKS - DATA_BLOCK_START)

#define SUPERBLOCK_MAGIC 0xD34D

typedef struct {
    uint16_t magic;
    uint32_t block_size;
    uint32_t total_blocks;
    uint32_t inode_bitmap_block;
    uint32_t data_bitmap_block;
    uint32_t inode_table_start;
    uint32_t first_data_block;
    uint32_t inode_size;
    uint32_t inode_count;
    uint8_t reserved[4058];
} __attribute__((packed)) Superblock;

typedef struct {
    uint32_t mode;
    uint32_t uid;
    uint32_t gid;
    uint32_t file_size;
    uint32_t atime;
    uint32_t ctime;
    uint32_t mtime;
    uint32_t dtime;
    uint32_t links;
    uint32_t blocks;
    uint32_t direct;
    uint32_t single_indirect;
    uint32_t double_indirect;
    uint32_t triple_indirect;
    uint8_t reserved[156];
} __attribute__((packed)) Inode;

void read_block(FILE *fp, int block_number, void *buffer) {
    fseek(fp, block_number * BLOCK_SIZE, SEEK_SET);
    fread(buffer, BLOCK_SIZE, 1, fp);
}

void write_block(FILE *fp, int block_number, const void *buffer) {
    fseek(fp, block_number * BLOCK_SIZE, SEEK_SET);
    fwrite(buffer, BLOCK_SIZE, 1, fp);
}

void read_inode(FILE *fp, int index, Inode *inode) {
    int block_offset = index / INODES_PER_BLOCK;
    int inode_offset = index % INODES_PER_BLOCK;
    fseek(fp, (INODE_TABLE_START + block_offset) * BLOCK_SIZE + inode_offset * INODE_SIZE, SEEK_SET);
    fread(inode, sizeof(Inode), 1, fp);
}

void write_inode(FILE *fp, int index, Inode *inode) {
    int block_offset = index / INODES_PER_BLOCK;
    int inode_offset = index % INODES_PER_BLOCK;
    fseek(fp, (INODE_TABLE_START + block_offset) * BLOCK_SIZE + inode_offset * INODE_SIZE, SEEK_SET);
    fwrite(inode, sizeof(Inode), 1, fp);
}

int is_block_used(uint8_t *bitmap, int block_index) {
    return (bitmap[block_index / 8] >> (block_index % 8)) & 1;
}

void set_block_used(uint8_t *bitmap, int block_index) {
    bitmap[block_index / 8] |= (1 << (block_index % 8));
}

void clear_block_used(uint8_t *bitmap, int block_index) {
    bitmap[block_index / 8] &= ~(1 << (block_index % 8));
}

int main(int argc, char *argv[]) {
    int fix_mode = 0;
    char *filename = NULL;

    if (argc == 2) {
        filename = argv[1];
    } else if (argc == 3 && strcmp(argv[1], "--fix") == 0) {
        fix_mode = 1;
        filename = argv[2];
    } else {
        fprintf(stderr, "Usage: %s [--fix] vsfs.img\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(filename, fix_mode ? "r+b" : "rb");
    if (!fp) {
        perror("Error opening file");
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    long filesize = ftell(fp);
    rewind(fp);

    if (filesize < TOTAL_BLOCKS * BLOCK_SIZE) {
        printf("Error: vsfs.img is too small: %ld bytes\n", filesize);
        fclose(fp);
        return 1;
    }

    Superblock sb;
read_block(fp, 0, &sb);
int sb_changed = 0;

printf("Validating Superblock...\n");

if (sb.magic != SUPERBLOCK_MAGIC) {
    printf("ERROR: Invalid magic number: 0x%x\n", sb.magic);
    if (fix_mode) { sb.magic = SUPERBLOCK_MAGIC; sb_changed = 1; printf("  --> Fixed\n"); }
}
if (sb.block_size != BLOCK_SIZE) {
    printf("ERROR: Block size is %u\n", sb.block_size);
    if (fix_mode) { sb.block_size = BLOCK_SIZE; sb_changed = 1; printf("  --> Fixed\n"); }
}
if (sb.total_blocks != TOTAL_BLOCKS) {
    printf("ERROR: Total blocks is %u\n", sb.total_blocks);
    if (fix_mode) { sb.total_blocks = TOTAL_BLOCKS; sb_changed = 1; printf("  --> Fixed\n"); }
}
if (sb.inode_bitmap_block != 1) {
    printf("ERROR: Inode bitmap block is %u\n", sb.inode_bitmap_block);
    if (fix_mode) { sb.inode_bitmap_block = 1; sb_changed = 1; printf("  --> Fixed\n"); }
}
if (sb.data_bitmap_block != 2) {
    printf("ERROR: Data bitmap block is %u\n", sb.data_bitmap_block);
    if (fix_mode) { sb.data_bitmap_block = 2; sb_changed = 1; printf("  --> Fixed\n"); }
}
if (sb.inode_table_start != INODE_TABLE_START) {
    printf("ERROR: Inode table start is %u\n", sb.inode_table_start);
    if (fix_mode) { sb.inode_table_start = INODE_TABLE_START; sb_changed = 1; printf("  --> Fixed\n"); }
}
if (sb.first_data_block != DATA_BLOCK_START) {
    printf("ERROR: First data block is %u\n", sb.first_data_block);
    if (fix_mode) { sb.first_data_block = DATA_BLOCK_START; sb_changed = 1; printf("  --> Fixed\n"); }
}
if (sb.inode_size != INODE_SIZE) {
    printf("ERROR: Inode size is %u\n", sb.inode_size);
    if (fix_mode) { sb.inode_size = INODE_SIZE; sb_changed = 1; printf("  --> Fixed\n"); }
}
if (sb.inode_count > MAX_INODES) {
    printf("Warning: inode_count (%u) exceeds max (%d). Clamping.\n", sb.inode_count, MAX_INODES);
    if (fix_mode) { sb.inode_count = MAX_INODES; sb_changed = 1; printf("  --> Fixed\n"); }
}

if (fix_mode && sb_changed) {
    printf("Writing fixed superblock to disk...\n");
    write_block(fp, 0, &sb);
}

    uint8_t inode_bitmap[BLOCK_SIZE];
    uint8_t data_bitmap[BLOCK_SIZE];
    uint8_t seen_data_blocks[BLOCK_SIZE] = {0};

    read_block(fp, sb.inode_bitmap_block, inode_bitmap);
    read_block(fp, sb.data_bitmap_block, data_bitmap);

    printf("Checking Inodes and Bitmaps...\n");

    for (int i = 0; i < MAX_INODES; i++) {
        Inode inode;
        read_inode(fp, i, &inode);

        int valid_inode = (inode.links > 0 && inode.dtime == 0);
        int marked = is_block_used(inode_bitmap, i);

        if (marked && !valid_inode) {
            printf("Warning: Inode %d marked used but invalid\n", i);
            if (fix_mode) {
                clear_block_used(inode_bitmap, i);
                printf("  --> Fixed: Inode bitmap cleared\n");
            }
        } else if (!marked && valid_inode) {
            printf("Warning: Inode %d not marked but valid\n", i);
            if (fix_mode) {
                set_block_used(inode_bitmap, i);
                printf("  --> Fixed: Inode bitmap set\n");
            }
        }

        if (!valid_inode)
            continue;

        if (inode.direct >= DATA_BLOCK_START && inode.direct < TOTAL_BLOCKS) {
            int rel_block = inode.direct - DATA_BLOCK_START;

            if (!is_block_used(data_bitmap, rel_block)) {
                printf("Error: Data block %u used by inode %d not marked in bitmap\n", inode.direct, i);
                if (fix_mode) {
                    set_block_used(data_bitmap, rel_block);
                    printf("  --> Fixed: Data bitmap set for block %u\n", inode.direct);
                }
            }

            if (is_block_used(seen_data_blocks, rel_block)) {
                printf("Error: Duplicate data block %u used in inode %d\n", inode.direct, i);
                if (fix_mode) {
                    inode.direct = 0;
                    write_inode(fp, i, &inode);
                    printf("  --> Fixed: Inode %d direct pointer cleared\n", i);
                }
            } else {
                set_block_used(seen_data_blocks, rel_block);
            }
        } else if (inode.direct != 0) {
            printf("Error: Invalid direct pointer %u in inode %d\n", inode.direct, i);
            if (fix_mode) {
                inode.direct = 0;
                write_inode(fp, i, &inode);
                printf("  --> Fixed: Cleared invalid direct pointer in inode %d\n", i);
            }
        }
    }

    for (int i = 0; i < DATA_BLOCK_COUNT; i++) {
        if (is_block_used(data_bitmap, i) && !is_block_used(seen_data_blocks, i)) {
            printf("Warning: Data block %d marked used but unreferenced\n", i + DATA_BLOCK_START);
            if (fix_mode) {
                clear_block_used(data_bitmap, i);
                printf("  --> Fixed: Cleared data bitmap for block %d\n", i + DATA_BLOCK_START);
            }
        }
    }

    if (fix_mode) {
        write_block(fp, sb.inode_bitmap_block, inode_bitmap);
        write_block(fp, sb.data_bitmap_block, data_bitmap);
        printf("All detected issues fixed and written to disk.\n");
    }

    printf("VSFS Check Completed.\n");
    fclose(fp);
    return 0;
}


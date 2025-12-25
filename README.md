# VSFSck-Consistency-Checker-for-Very-Simple-File-System-VSFS-
VSFSck is a file system consistency checker designed to verify and repair a custom virtual file system called VSFS. This project demonstrates deep understanding of file system internals by analyzing and fixing corrupted file system images.
# Core Validation Checks
Superblock Integrity: Validates magic number (0xD34D), block size, total blocks, and key metadata pointers

Bitmap Consistency: Ensures inode and data bitmaps accurately reflect actual usage

Block Reference Tracking: Detects duplicate and out-of-range block references

# Advanced Functionality
Cross-Validation: Verifies bidirectional consistency between bitmaps and actual allocations

Inode Validity Checking: Validates inode metadata including link counts and deletion times

Error Detection & Reporting: Identifies all structural inconsistencies in the corrupted file system image

# File System Specifications
Block Size: 4096 bytes

Total Blocks: 64 blocks

Inode Size: 256 bytes

Inode Count: Supports multiple inodes across 5 inode table blocks

Layout: Organized structure with dedicated blocks for superblock, bitmaps, inode table, and data

# Project Objectives
Analyze corrupted VSFS images for structural issues

Detect inconsistencies in superblock, bitmaps, inodes, and data blocks

Repair identified errors to restore file system integrity

Validate the corrected image passes all consistency checks

# Technical Implementation
The project involves low-level file system manipulation, including:

Direct byte-level access to file system structures

Bitmap operations and validation

Inode metadata parsing and verification

Block reference tracking and validation

This tool serves as an educational implementation of file system repair utilities similar to fsck, providing hands-on experience with file system architecture and recovery techniques.

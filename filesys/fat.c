#include "filesys/fat.h"
#include "devices/disk.h"
#include "filesys/filesys.h"

#include "threads/malloc.h"
#include "threads/synch.h"
#include <stdio.h>
#include <string.h>

#define data_start(FATBS) (1 + ((FATBS).fat_sectors))
#define data_sectors(FATBS) (((FATBS).total_sectors) - (data_start (FATBS)))

/* Should be less than DISK_SECTOR_SIZE */
struct fat_boot {
	unsigned int magic;
	unsigned int sectors_per_cluster; /* Fixed to 1 */
	unsigned int total_sectors;
	unsigned int fat_start;
	unsigned int fat_sectors; /* Size of FAT in sectors. */
	unsigned int root_dir_cluster;
};

/* FAT FS */
struct fat_fs {
	struct fat_boot bs;
	unsigned int *fat;
	unsigned int fat_length;
	disk_sector_t data_start;
	cluster_t last_clst;
	cluster_t to_add;
	struct lock write_lock;
};

static struct fat_fs *fat_fs;

void fat_boot_create (void);
void fat_fs_init (void);

void
fat_init (void) {
	fat_fs = calloc (1, sizeof (struct fat_fs));
	if (fat_fs == NULL)
		PANIC ("FAT init failed");

	// Read boot sector from the disk
	unsigned int *bounce = malloc (DISK_SECTOR_SIZE);
	if (bounce == NULL)
		PANIC ("FAT init failed");
	disk_read (filesys_disk, FAT_BOOT_SECTOR, bounce);
	memcpy (&fat_fs->bs, bounce, sizeof (fat_fs->bs));
	free (bounce);

	// Extract FAT info
	if (fat_fs->bs.magic != FAT_MAGIC)
		fat_boot_create ();
	fat_fs_init ();
}

void
fat_open (void) {
	fat_fs->fat = calloc (fat_fs->fat_length, sizeof (cluster_t));
	if (fat_fs->fat == NULL)
		PANIC ("FAT load failed");

	// Load FAT directly from the disk
	uint8_t *buffer = (uint8_t *) fat_fs->fat;
	off_t bytes_read = 0;
	off_t bytes_left = sizeof (fat_fs->fat);
	const off_t fat_size_in_bytes = fat_fs->fat_length * sizeof (cluster_t);
	for (unsigned i = 0; i < fat_fs->bs.fat_sectors; i++) {
		bytes_left = fat_size_in_bytes - bytes_read;
		if (bytes_left >= DISK_SECTOR_SIZE) {
			disk_read (filesys_disk, fat_fs->bs.fat_start + i,
			           buffer + bytes_read);
			bytes_read += DISK_SECTOR_SIZE;
		} else {
			uint8_t *bounce = malloc (DISK_SECTOR_SIZE);
			if (bounce == NULL)
				PANIC ("FAT load failed");
			disk_read (filesys_disk, fat_fs->bs.fat_start + i, bounce);
			memcpy (buffer + bytes_read, bounce, bytes_left);
			bytes_read += bytes_left;
			free (bounce);
		}
	}
}

void
fat_close (void) {
	//printf("fat_close...\n");
	// Write FAT boot sector
	uint8_t *bounce = calloc (1, DISK_SECTOR_SIZE);
	if (bounce == NULL)
		PANIC ("FAT close failed");
	memcpy (bounce, &fat_fs->bs, sizeof (fat_fs->bs));
	disk_write (filesys_disk, FAT_BOOT_SECTOR, bounce);
	free (bounce);

	// Write FAT directly to the disk
	uint8_t *buffer = (uint8_t *) fat_fs->fat;
	off_t bytes_wrote = 0;
	off_t bytes_left = sizeof (fat_fs->fat);
	const off_t fat_size_in_bytes = fat_fs->fat_length * sizeof (cluster_t);
	for (unsigned i = 0; i < fat_fs->bs.fat_sectors; i++) {
		//printf("fat_fs->bs.fat_sectors : %u\ti : %u\n", fat_fs->bs.fat_sectors, i);
		if (i == 0) continue;
		bytes_left = fat_size_in_bytes - bytes_wrote;
		if (bytes_left >= DISK_SECTOR_SIZE) {
			disk_write (filesys_disk, fat_fs->bs.fat_start + i,
			            buffer + bytes_wrote);
			bytes_wrote += DISK_SECTOR_SIZE;
		} else {
			bounce = calloc (1, DISK_SECTOR_SIZE);
			if (bounce == NULL)
				PANIC ("FAT close failed");
			memcpy (bounce, buffer + bytes_wrote, bytes_left);
			disk_write (filesys_disk, fat_fs->bs.fat_start + i, bounce);
			bytes_wrote += bytes_left;
			free (bounce);
		}
	}
}

void
fat_create (void) {
	// Create FAT boot
	fat_boot_create ();
	fat_fs_init ();

	// Create FAT table
	//printf("\nlen : %u\tcluster size : %u\n", fat_fs->fat_length, sizeof (cluster_t));
	fat_fs->fat = calloc (fat_fs->fat_length, sizeof (cluster_t));
	if (fat_fs->fat == NULL)
		PANIC ("FAT creation failed");

	// Set up ROOT_DIR_CLST
	fat_put (ROOT_DIR_CLUSTER, EOChain);

	// Fill up ROOT_DIR_CLUSTER region with 0
	uint8_t *buf = calloc (1, DISK_SECTOR_SIZE);
	if (buf == NULL)
		PANIC ("FAT create failed due to OOM");
	//printf("!!root writing!!\n");
	disk_write (filesys_disk, cluster_to_sector (ROOT_DIR_CLUSTER), buf);
	//printf("!!root writing!!\n");
	free (buf);
}

void
fat_boot_create (void) {
	unsigned int fat_sectors =
	    (disk_size (filesys_disk) - 1)
	    / (DISK_SECTOR_SIZE / sizeof (cluster_t) * SECTORS_PER_CLUSTER + 1) + 1;
	fat_fs->bs = (struct fat_boot){
	    .magic = FAT_MAGIC,
	    .sectors_per_cluster = SECTORS_PER_CLUSTER,
	    .total_sectors = disk_size (filesys_disk),
	    .fat_start = 1,
	    .fat_sectors = fat_sectors,
	    .root_dir_cluster = ROOT_DIR_CLUSTER,
	};
}

void
fat_fs_init (void) {
	/* TODO: Your code goes here. */
	fat_fs->data_start = data_start(fat_fs->bs);
	fat_fs->fat_length = fat_fs->bs.total_sectors;
	fat_fs->last_clst = ROOT_DIR_CLUSTER;
	fat_fs->to_add = 0;
}

/*----------------------------------------------------------------------------*/
/* FAT handling                                                               */
/*----------------------------------------------------------------------------*/

static cluster_t find_back(cluster_t clst){
	int i = 2;
	//printf("input : %u\n", clst);
	if(clst == 0 || clst == 1) return 0;
	unsigned int *fat = fat_fs->fat;
	while(i<=fat_fs->last_clst){
		if(fat_get(i) == clst){
			//printf("return : %u\n", i);
			return i;
		}
		i++;
	}
	return 0;
}

/* Add a cluster to the chain.
 * If CLST is 0, start a new chain.
 * Returns 0 if fails to allocate a new cluster. */
cluster_t
fat_create_chain (cluster_t clst) {
	/* TODO: Your code goes here. */
	if(clst < 2) {
		//printf("\tto_add = 0 reset\n");
		fat_fs->to_add = 0;
		clst = 1;
	}
	
	unsigned int *fat = fat_fs->fat;
	int i = ROOT_DIR_CLUSTER + 1, cnt = 1;
	bool look_back = false;
	cluster_t saved = 0, return_value = 0;
	if(fat_fs->to_add != 0) {
		saved = fat_fs->to_add;
		look_back = true;
	}
	while(cnt <= clst){
		if (fat_get(i)==0){
			cnt++;
			if(saved) fat_put(saved, i);
			if(!return_value) return_value = i;
			saved = i;
			if(i > fat_fs->last_clst) fat_fs->last_clst = i;
		}
		i++;
	}
	fat_put(saved, EOChain);
	//fat_fs->to_add = saved;
	if(look_back){
		cluster_t temp = find_back(return_value);
		while(temp){
			return_value = temp;
			temp = find_back(return_value);
		}
	}
	//printf("fat_create_chain...\n\tstart : %u\tlast : %u\n\tinput : %u\tto_add : %u\tlast_clst : %u\n", return_value, saved, clst, fat_fs->to_add, fat_fs->last_clst);
	return return_value;
}

/* Remove the chain of clusters starting from CLST.
 * If PCLST is 0, assume CLST as the start of the chain. */
void
fat_remove_chain (cluster_t clst, cluster_t pclst) {
	/* TODO: Your code goes here. */
	//printf("fat_remove_chain...\n\tstart : %u\tbefore : %u\n", clst, pclst);
	if (clst == ROOT_DIR_CLUSTER) PANIC("CANNOT REMOVE ROOT");
	unsigned int *fat = fat_fs->fat;
	cluster_t cur = clst;
	while(fat_get(cur) != EOChain){
		cluster_t next = fat_get(cur);
		if(next == 0)return;
		fat_put(cur, 0);
		cur = next;
	}
	fat_put(cur, 0);
	while(fat_get(fat_fs->last_clst) == 0){
		fat_fs->last_clst--;
	}
	if(pclst <= 1) return;
	fat_put(pclst, EOChain);
}

/* Update a value in the FAT table. */
void
fat_put (cluster_t clst, cluster_t val) {
	/* TODO: Your code goes here. */
	unsigned int *fat = fat_fs->fat;
	fat[clst] = val;
}

/* Fetch a value in the FAT table. */
cluster_t
fat_get (cluster_t clst) {
	/* TODO: Your code goes here. */
	//printf("get : %u\n", clst);
	if(fat_fs->fat_length < clst){
		printf("clst : %x\n", clst);
		PANIC("END");
	}
	unsigned int *fat = fat_fs->fat;
	return fat[clst];
}

/* Covert a cluster # to a sector number. */
disk_sector_t
cluster_to_sector (cluster_t clst) {
	/* TODO: Your code goes here. */
	return clst;
}

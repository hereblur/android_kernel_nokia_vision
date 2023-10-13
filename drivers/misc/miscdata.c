/*
 * Copyright (C) 2021 wingtech Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/of_fdt.h>
#include <asm/setup.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/genhd.h>
#include <linux/kernel.h>
#include <linux/utsname.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/file.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/blkdev.h>
#include <linux/buffer_head.h>
#include <linux/kdev_t.h>


extern struct gendisk *emmc_disk;
static struct platform_device *proinfo_pdev;
static struct device *proinfo_root_dev;

//define type
typedef enum{
    WT_PROINFO_factoryreset_date = 0,
}wt_proinfo_type;

#define MISCDATA_PATH "/dev/block/by-name/miscdata"//miscdata partition
#define BASE_ADDR 1024*768//customer base addr.defined by sprd
#define OFFSET 341//341-404 save the shutdown time when recovery factory setup
#define SIZE 64//341-404 save the shutdown time when recovery factory setup
#define EMMC_BLOCK_LENGTH 512//emmc read or write suggest

static int wt_proinfo_read(wt_proinfo_type type, char* buf);
static int wt_proinfo_write(wt_proinfo_type type, const char* buf, int len);

//define macor ATTR
#define WT_PROINFO_CREATE_ATTR(name) \
static ssize_t proinfo_##name##_store(struct device * dev, struct device_attribute *attr, const char * buf,size_t count); \
static ssize_t proinfo_##name##_show(struct device *dev, struct device_attribute *attr, char *buf); \
\
\
static ssize_t proinfo_##name##_store(struct device * dev, struct device_attribute *attr, const char * buf,\
                  size_t count)\
{\
    int ret = -1;\
    printk("wt_proinfo_write entry  %s\n",__FUNCTION__);\
\
    ret = wt_proinfo_write(WT_PROINFO_##name, buf, count);\
    \
    return count;\
}\
\
static ssize_t proinfo_##name##_show(struct device *dev, struct device_attribute *attr, char *buf)\
{\
    int ret = -1;\
    char pbuf[SIZE];\
    printk("wt_proinfo_read entry  %s\n",__FUNCTION__);\
\
    ret = wt_proinfo_read(WT_PROINFO_##name,pbuf);\
\
    return sprintf(buf, "%s\n",pbuf);\
}\
\
static DEVICE_ATTR(name, S_IWUSR|S_IRUSR|S_IROTH|S_IRGRP, proinfo_##name##_show, proinfo_##name##_store);

//define function
WT_PROINFO_CREATE_ATTR(factoryreset_date)

//+ExtR IRU-838,baiyun1.wt,ADD,20210312,read the remote lock flag from miscdata partition
//+ExtR IRU-842,baiyun1.wt,ADD,20210312,write the remote lock flag from miscdata partition
#define BUF_SIZE 1024

#define REMOTE_LOCK_OFFSET  (85)
#define REMOTE_LOCK_SIZE    (340-85+1)

//Added by jiangsuting.wt for SCT-719,Block/Unlock hardward factory reset option on 20210930 begin
#define BLOCK_FACTORY_RESET_OFFSET (459)
#define BLOCK_FACTORY_RESET_SIZE  (1)
//Added by jiangsuting.wt for SCT-719,Block/Unlock hardward factory reset option on 20210930 end

//Added by jiangsuting.wt for SCT-720,Block/unblock Device getting into fastboot on 20210930 begin
#define BLOCK_FASTBOOT_MODE_OFFSET (460)
#define BLOCK_FASTBOOT_MODE_SIZE  (1)
//Added by jiangsuting.wt for SCT-720,Block/unblock Device getting into fastboot on 20210930 end
//define type
typedef enum{
    WT_PROINFO_EX_remote_lock_value = 0,
    WT_PROINFO_EX_block_factory_reset = 1,
    WT_PROINFO_EX_block_fastboot_mode = 2,
} wt_proinfo_type_ex;

static int wt_proinfo_read_ex(wt_proinfo_type_ex type, char* buf);
static int wt_proinfo_write_ex(wt_proinfo_type_ex type, const char* buf, int len);

//define macor ATTR
#define WT_PROINFO_CREATE_ATTR_EX(name) \
static ssize_t proinfo_##name##_ex_store(struct device * dev, struct device_attribute *attr, const char * buf,size_t count); \
static ssize_t proinfo_##name##_ex_show(struct device *dev, struct device_attribute *attr, char *buf); \
\
\
static ssize_t proinfo_##name##_ex_store(struct device * dev, struct device_attribute *attr, const char * buf,\
                  size_t count)\
{\
    int ret = -1;\
    printk("wt_proinfo_write_ex entry  %s\n",__FUNCTION__);\
\
    ret = wt_proinfo_write_ex(WT_PROINFO_EX_##name, buf, count);\
    \
    return (ret < 0) ? ret : count ;\
}\
\
static ssize_t proinfo_##name##_ex_show(struct device *dev, struct device_attribute *attr, char *buf)\
{\
    int ret = -1;\
    char pbuf[BUF_SIZE];\
    memset(pbuf, 0, sizeof(pbuf));\
    printk("wt_proinfo_read_ex entry  %s\n",__FUNCTION__);\
\
    ret = wt_proinfo_read_ex(WT_PROINFO_EX_##name,pbuf);\
\
    return (ret < 0) ? ret : sprintf(buf, "%s\n",pbuf);\
}\
\
static DEVICE_ATTR(name, S_IWUSR|S_IRUSR|S_IROTH|S_IRGRP, proinfo_##name##_ex_show, proinfo_##name##_ex_store);

//define function
WT_PROINFO_CREATE_ATTR_EX(remote_lock_value)
WT_PROINFO_CREATE_ATTR_EX(block_factory_reset)
WT_PROINFO_CREATE_ATTR_EX(block_fastboot_mode)

//-ExtR IRU-838,baiyun1.wt,ADD,20210312,read the remote lock flag from miscdata partition
//-ExtR IRU-842,baiyun1.wt,ADD,20210312,write the remote lock flag from miscdata partition



static int wt_create_device_files(void)
{
    int rc = 0;
    rc = device_create_file(proinfo_root_dev, &dev_attr_factoryreset_date);
    if (rc)
        return rc;
//+ExtR IRU-838,baiyun1.wt,ADD,20210312,read the remote lock flag from miscdata partition
//+ExtR IRU-842,baiyun1.wt,ADD,20210312,write the remote lock flag from miscdata partition
    rc = device_create_file(proinfo_root_dev, &dev_attr_remote_lock_value);
    if (rc)
        return rc;
//-ExtR IRU-838,baiyun1.wt,ADD,20210312,read the remote lock flag from miscdata partition
//-ExtR IRU-842,baiyun1.wt,ADD,20210312,write the remote lock flag from miscdata partition

//Added by jiangsuting.wt for SCT-719,Block/Unlock hardward factory reset option on 20210930 begin
    rc = device_create_file(proinfo_root_dev, &dev_attr_block_factory_reset);
    if (rc)
        return rc;
//Added by jiangsuting.wt for SCT-719,Block/Unlock hardward factory reset option on 20210930 end
//Added by jiangsuting.wt for SCT-720,Block/unblock Device getting into fastboot on 20210930 begin
    rc = device_create_file(proinfo_root_dev, &dev_attr_block_fastboot_mode);
    if (rc)
        return rc;
//Added by jiangsuting.wt for SCT-720,Block/unblock Device getting into fastboot on 20210930 end
    return 0;
}

#define MMC_BLOCK_SIZE (512)
static dev_t emmc_lookup_partition(const char *part_name, sector_t *start, sector_t *nr_sect)
{
	struct disk_part_iter piter;
	struct hd_struct *part;

	dev_t devt = MKDEV(0, 0);
	if (!emmc_disk) {
		printk("[mzss] emmc disk = null\n");

		return devt;
	}

	disk_part_iter_init(&piter, emmc_disk, DISK_PITER_INCL_EMPTY);
	while ((part = disk_part_iter_next(&piter))) {
		if (part->info && !strcmp(part->info->volname, part_name)) {
			devt = part->__dev.devt;
			*start = part->start_sect;
			*nr_sect = part->nr_sects;
			break;
		}
	}
	disk_part_iter_exit(&piter);

	return devt;
}

static int emmc_block_rw(int write, sector_t index, void *buffer, size_t len)
{
	struct block_device *bdev;
	struct buffer_head *bh = NULL;
	fmode_t mode = FMODE_READ;
	int err = -EIO;

	if (len > MMC_BLOCK_SIZE)
		return -EINVAL;

	bdev = bdget(MKDEV(MMC_BLOCK_MAJOR, 0));
	if (!bdev)
		return -EIO;

	mode = write ? FMODE_WRITE : FMODE_READ;
	if (blkdev_get(bdev, mode, NULL)) {
		bdput(bdev);
		goto out;
	}

	set_blocksize(bdev, MMC_BLOCK_SIZE);

	bh = __getblk(bdev, index, MMC_BLOCK_SIZE);

	if (bh) {
		clear_buffer_uptodate(bh);
		get_bh(bh);
		lock_buffer(bh);
		bh->b_end_io = end_buffer_read_sync;
		submit_bh(REQ_OP_READ, 0, bh);
		wait_on_buffer(bh);
		pr_err("emmc read sucess!!\n");
		if (unlikely(!buffer_uptodate(bh))) {
			pr_err("emmc read error!!\n");
			goto out;
		}
		if (write) {
			lock_buffer(bh);
			memcpy(bh->b_data, buffer, len);
			bh->b_end_io = end_buffer_write_sync;
			get_bh(bh);
			submit_bh(REQ_OP_WRITE, 1, bh);
			wait_on_buffer(bh);
			pr_err("emmc go to write sucess!!\n");
			if (unlikely(!buffer_uptodate(bh))) {
				pr_err("emmc write error!!\n");
				goto out;
			}
		} else {
			memcpy(buffer, bh->b_data, len);
			pr_err("chenjindongemmc write sucess!!\n");
		}
		err = 0;
	} else {
		pr_info("%s error\n", __func__);
	}

out:
	brelse(bh);
	blkdev_put(bdev, mode);

	return err;
}

int emmc_partition_rw(const char *part_name, int write, loff_t offset,void *buffer, size_t len)
{
	int ret = 0;
	sector_t index;
	void *p = buffer;

	dev_t devt;
	sector_t start=0, nr_sect=0;

	if (buffer == NULL)
		return -EINVAL;
	printk("[mzss123]%s: offset(%lld) unalign to 512Byte!\n", __func__, offset);
	if (offset % MMC_BLOCK_SIZE) {
		printk("[mzss]%s: offset(%lld) unalign to 512Byte!\n", __func__, offset);
		return -EINVAL;
	}

	devt = emmc_lookup_partition(part_name, &start, &nr_sect);
	if (!devt) {
		printk("[mzss]%s: can't find eMMC partition(%s)\n", __func__, part_name);
		return -ENODEV;
	}

	if (offset < 0 || (offset + len) >= nr_sect * MMC_BLOCK_SIZE) {
		printk("[mzss]%s: access area exceed parition(%s) range.\n", __func__, part_name);
		return -EINVAL;
	}

	index = start + offset / MMC_BLOCK_SIZE;

	while (len > 0) {
		size_t size = len;

		if (size > MMC_BLOCK_SIZE)
			size = MMC_BLOCK_SIZE;

		ret = emmc_block_rw(write, index, p, size);
		if (ret) {
			printk("[mzss]%s (%lu) error %d\n", __func__, (unsigned long)len, ret);
			break;
		}

		len -= size;
		index++;
		p += MMC_BLOCK_SIZE;
	}

	return ret;
}
EXPORT_SYMBOL(emmc_partition_rw);


static int wt_proinfo_read(wt_proinfo_type type, char* buf)
{
    //struct file *fp = NULL;
    char fbuf[EMMC_BLOCK_LENGTH]={0};
    int ret= 0;
    #if 1
    ret = emmc_partition_rw("miscdata",  0, BASE_ADDR, (char *) fbuf, (unsigned long) EMMC_BLOCK_LENGTH);
    strncpy(buf, fbuf + OFFSET, SIZE);
    #else
    fp = filp_open(MISCDATA_PATH, O_RDWR | O_CREAT, 0);
    if (IS_ERR(fp)){
        printk("[TYPE] %s: open miscdata path error\n", __func__);
           return -1;
    }

    fp->f_pos = fp->f_pos + BASE_ADDR + OFFSET;
    //printk("read %d\n", fp->f_pos );
    ret = kernel_read(fp,  fbuf, SIZE, &fp->f_pos);
    if (ret < 0)
    {
        printk("%s: Read bytes from miscdata failed! %d\n", __func__, ret);
        filp_close(fp, NULL);
        return -1;
    }
    #endif
    printk("%s:read result is %s\n",__func__, buf);
    //memcpy(buf, fbuf, SIZE);
    //filp_close(fp, NULL);

    return ret;
}

//store in buf, return len. buf least size is 1024
static int wt_proinfo_write(wt_proinfo_type type, const char* buf, int len)
{
    //struct file *fp = NULL;
    int ret = 0;
    char buf_tmp[EMMC_BLOCK_LENGTH] = {0};

    /*memcpy(buf_tmp, buf, len);
    if(buf_tmp[len-1] == '\n')
        buf_tmp[len-1] = 0x00;*/
    #if 1
    ret = emmc_partition_rw("miscdata",  0, BASE_ADDR, (char *) buf_tmp, (unsigned long) EMMC_BLOCK_LENGTH);
    if (ret != 0)
        printk("%s:read miscdata error\n", __func__);

    if (len > SIZE) {
        printk("%s:the len is larger than 64!\n", __func__);
        return ret;
    }

    strncpy(buf_tmp + OFFSET, buf, len);

    ret = emmc_partition_rw("miscdata", 1, BASE_ADDR, (char *) buf_tmp, (unsigned long) EMMC_BLOCK_LENGTH);
    #else
    fp = filp_open(MISCDATA_PATH, O_RDWR | O_CREAT, 0);
    if (IS_ERR(fp)){
        printk("[RTX] %s: open miscdata path error\n", __func__);
        return -1;
    }

    fp->f_pos = fp->f_pos + BASE_ADDR + OFFSET;
    //printk("write %d\n", fp->f_pos );
    ret = kernel_write(fp, buf_tmp, (unsigned long) len, &fp->f_pos);
    if (ret < 0) {
        printk("%s: write bytes to miscdata failed! %d\n", __func__, ret);
        filp_close(fp, NULL);
        return -1;
    }

    filp_close(fp, NULL);
    #endif
    printk("%s,write %s.the length is %d\n", __func__, buf, len);

    return ret;
}

//+ExtR IRU-838,baiyun1.wt,ADD,20210312,read the remote lock flag from miscdata partition
//+ExtR IRU-842,baiyun1.wt,ADD,20210312,write the remote lock flag from miscdata partition
static int wt_proinfo_read_ex(wt_proinfo_type_ex type, char* buf)
{
    int ret = -1;
    char buf_tmp[EMMC_BLOCK_LENGTH] = {0};
    int size;
    int offset;

    switch(type) {
            case(WT_PROINFO_EX_remote_lock_value) :
                size = REMOTE_LOCK_SIZE;
                offset = REMOTE_LOCK_OFFSET;
            break;
//Added by jiangsuting.wt for SCT-719,Block/Unlock hardward factory reset option on 20210930 begin
            case(WT_PROINFO_EX_block_factory_reset) :
                size = BLOCK_FACTORY_RESET_SIZE ;
                offset = BLOCK_FACTORY_RESET_OFFSET ;
            break;
//Added by jiangsuting.wt for SCT-719,Block/Unlock hardward factory reset option on 20210930 end
//Added by jiangsuting.wt for SCT-720,Block/unblock Device getting into fastboot on 20210930 begin
            case(WT_PROINFO_EX_block_fastboot_mode) :
                size = BLOCK_FASTBOOT_MODE_SIZE ;
                offset = BLOCK_FASTBOOT_MODE_OFFSET ;
            break;
//Added by jiangsuting.wt for SCT-720,Block/unblock Device getting into fastboot on 20210930 end
            default:
                printk("%s: no type error!\n", __func__);
                return ret;
            break;
    }

    ret = emmc_partition_rw("miscdata",  0, BASE_ADDR, (char *) buf_tmp, (unsigned long) EMMC_BLOCK_LENGTH);
    if (ret != 0)
        printk("%s:read miscdata error\n", __func__);

    strncpy(buf, buf_tmp + offset, size);

    printk("%s:read result is %s\n",__func__, buf);

    return ret;
}

//store in buf, return len. buf least size is 1024
static int wt_proinfo_write_ex(wt_proinfo_type_ex type, const char* buf, int len)
{
    int ret = -1;
    char buf_tmp[EMMC_BLOCK_LENGTH] = {0};
    int size;
    int offset;

    switch(type) {
            case(WT_PROINFO_EX_remote_lock_value) :
                size = REMOTE_LOCK_SIZE;
                offset = REMOTE_LOCK_OFFSET;
            break;
//Added by jiangsuting.wt for SCT-719,Block/Unlock hardward factory reset option on 20210930 begin
            case(WT_PROINFO_EX_block_factory_reset) :
                size = BLOCK_FACTORY_RESET_SIZE ;
                offset = BLOCK_FACTORY_RESET_OFFSET ;
            break;
//Added by jiangsuting.wt for SCT-719,Block/Unlock hardward factory reset option on 20210930 end
//Added by jiangsuting.wt for SCT-720,Block/unblock Device getting into fastboot on 20210930 begin
            case(WT_PROINFO_EX_block_fastboot_mode) :
                size = BLOCK_FASTBOOT_MODE_SIZE ;
                offset = BLOCK_FASTBOOT_MODE_OFFSET ;
            break;
//Added by jiangsuting.wt for SCT-720,Block/unblock Device getting into fastboot on 20210930 end
            default:
                printk("%s: no type error!\n", __func__);
                return ret;
            break;
    }

    len = (len > size) ? size : len;

    ret = emmc_partition_rw("miscdata",  0, BASE_ADDR, (char *) buf_tmp, (unsigned long) EMMC_BLOCK_LENGTH);
    if (ret != 0) {
        printk("%s:read miscdata error\n", __func__);
        return ret;
    }

    strncpy(buf_tmp + offset, buf, len);

    ret = emmc_partition_rw("miscdata", 1, BASE_ADDR, (char *) buf_tmp, (unsigned long) EMMC_BLOCK_LENGTH);

    printk("%s,write %s.the length is %d\n", __func__, buf, len);

    return ret;
}
//-ExtR IRU-838,baiyun1.wt,ADD,20210312,read the remote lock flag from miscdata partition
//-ExtR IRU-842,baiyun1.wt,ADD,20210312,write the remote lock flag from miscdata partition

static struct platform_driver proinfo_pdrv = {
    .driver = {
    .name    = "proinfo",
    .owner    = THIS_MODULE,
    },
};

static int __init
proinfo_init(void)
{
    int rc;

    rc = platform_driver_register(&proinfo_pdrv);
    if (rc)
        return rc;

    proinfo_pdev = platform_device_register_simple("proinfo", -1, NULL,0);
    if (IS_ERR(proinfo_pdev)) {
        rc = PTR_ERR(proinfo_pdev);
        goto out_pdrv;
    }

    proinfo_root_dev = root_device_register("proinfo");
    if (IS_ERR(proinfo_root_dev)) {
        rc = PTR_ERR(proinfo_root_dev);
        goto out_pdev;
    }
    rc = wt_create_device_files();
    if (rc)
        goto out_root;

    return 0;

out_root:
    root_device_unregister(proinfo_root_dev);
out_pdev:
    platform_device_unregister(proinfo_pdev);
out_pdrv:
    platform_driver_unregister(&proinfo_pdrv);
    return rc;
}

/*
 * The init/exit functions.
 */
static void __exit
proinfo_exit(void)
{
    platform_device_unregister(proinfo_pdev);
    platform_driver_unregister(&proinfo_pdrv);
    root_device_unregister(proinfo_root_dev);
}

module_init(proinfo_init);
module_exit(proinfo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wt");


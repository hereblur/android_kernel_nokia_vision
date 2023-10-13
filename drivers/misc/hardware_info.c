//Added by wt.yanrenjie for SCT-702,wt hardware info begin
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/hardware_info.h>
#include <linux/platform_device.h>
#include <linux/of.h>

char Lcm_name[HARDWARE_MAX_ITEM_LONGTH];
//Modify by fangzhihua@wingtech.com for SCT-702,add for check panel for second TP FW on 2021-11-12 begin
#ifdef WT_COMPILE_FACTORY_VERSION
EXPORT_SYMBOL_GPL(Lcm_name);
#endif
//Modify by fangzhihua@wingtech.com for SCT-702,add for check panel for second TP FW on 2021-11-12 end
char Tp_info[HARDWARE_MAX_ITEM_LONGTH];
#ifdef WT_COMPILE_FACTORY_VERSION
EXPORT_SYMBOL_GPL(Tp_info);
#endif

char Sar_name[HARDWARE_MAX_ITEM_LONGTH];
char board_id[HARDWARE_MAX_ITEM_LONGTH];
char hardware_id[HARDWARE_MAX_ITEM_LONGTH];
static char hardwareinfo_name[HARDWARE_MAX_ITEM][HARDWARE_MAX_ITEM_LONGTH];

char* hardwareinfo_items[HARDWARE_MAX_ITEM] =
{
	"LCD",
	"TP",
	"MEMORY",
	"CAM_FRONT",
	"CAM_BACK",
	"BT",
	"WIFI",
	"GSENSOR",
	"PLSENSOR",
	"GYROSENSOR",
	"MSENSOR",
	"SAR",
	"GPS",
	"FM",
	"BATTERY",
	"PA",
	"CAM_M_BACK",
	"CAM_M_FRONT",
	"BOARD_ID",
	"HARDWARE_ID"
};

int hardwareinfo_set_prop(int cmd, const char *name)
{
	if(cmd < 0 || cmd >= HARDWARE_MAX_ITEM)
		return -1;

	strcpy(hardwareinfo_name[cmd], name);

	return 0;
}

int __weak tid_hardware_info_get(char *buf, int size)
{
	snprintf(buf, size, "touch info interface is not ready\n");

	return 0;
}
#ifdef WT_COMPILE_FACTORY_VERSION
EXPORT_SYMBOL_GPL(hardwareinfo_set_prop);
#endif
static char* boardid_get(void)
{
	char* s1= "";
	char* s2="not found";

	s1 = strstr(saved_command_line, "board_id=");
	if(!s1) {
		printk("board_id not found in cmdline\n");
		return s2;
	}
	s1 += strlen("board_id=");
	strncpy(board_id, s1, 9);
	board_id[10]='\0';
	s1 = board_id;
	//printk("board_id found in cmdline : %s\n", board_id);

	return s1;
}

static char* hwid_get(void)
{
	char* s1= "";
	char* s2="not found";
	char *ptr =NULL;

	s1 = strstr(saved_command_line, "hardware_id=");
	if(!s1) {
		printk("hardware_id not found in cmdline\n");
		return s2;
	}
	s1 += strlen("hardware_id=");
	ptr=s1;
	while(*ptr != ' ' && *ptr != '\0') {
		ptr++;
	}
	strncpy(hardware_id, (const char *)s1,ptr-s1);
	hardware_id[ptr-s1]='\0';
	/*printk("hardware_id found in cmdline : %s\n", hardware_id);
	if (strncmp(hardware_id, "EVT", strlen("hardware_id")) == 0)
	{
		strcpy(hardware_id, "REV0.1");
	}
	else if (strncmp(hardware_id, "EVT2", strlen("hardware_id")) == 0)
	{
		strcpy(hardware_id, "REV0.2");
	}
	else if (strncmp(hardware_id, "DVT", strlen("hardware_id")) == 0)
	{
		strcpy(hardware_id, "REV0.3");
	}
	else if (strncmp(hardware_id, "PVT", strlen("hardware_id")) == 0)
	{
		strcpy(hardware_id, "REV0.4");
	}
	else if (strncmp(hardware_id, "MP", strlen("hardware_id")) == 0)
	{
		strcpy(hardware_id, "MP1.0");
	}
	else
	{
		return s2;
	}*/
	s1 = hardware_id;

	return s1;
}

static char* lcd_name_get(void)
{
        struct device_node *cmdline_node;
        const char *cmd_line, *lcd_name_p;
        int rc;
        char* s1= "not found";

        cmdline_node = of_find_node_by_path("/chosen");
        rc = of_property_read_string(cmdline_node, "bootargs", &cmd_line);
        if (!rc) {
                lcd_name_p = strstr(cmd_line, "lcd_name=");
                if (lcd_name_p) {
                        memset(Lcm_name, 0, HARDWARE_MAX_ITEM_LONGTH);
                        sscanf(lcd_name_p, "lcd_name=%s", Lcm_name);
                        s1 = Lcm_name;
                        //printk("Lcm_name found in cmdline : %s\n", Lcm_name);
                }
        }

        return s1;
}

static long hardwareinfo_ioctl(struct file *file, unsigned int cmd,unsigned long arg)
{
	int ret = 0, hardwareinfo_num = HARDWARE_MAX_ITEM;
	void __user *data = (void __user *)arg;
	//char info[HARDWARE_MAX_ITEM_LONGTH];

	switch (cmd) {
	case HARDWARE_LCD_GET:
		hardwareinfo_set_prop(HARDWARE_LCD, Lcm_name);
		hardwareinfo_num = HARDWARE_LCD;
		break;
	case HARDWARE_TP_GET:
		hardwareinfo_num = HARDWARE_TP;
		strcpy(hardwareinfo_name[HARDWARE_TP],Tp_info);
		//tid_hardware_info_get(hardwareinfo_name[hardwareinfo_num],
		//ARRAY_SIZE(hardwareinfo_name[0]));
		break;
	case HARDWARE_FLASH_GET:
		hardwareinfo_num = HARDWARE_FLASH;
		break;
	case HARDWARE_FRONT_CAM_GET:
		hardwareinfo_num = HARDWARE_FRONT_CAM;
		break;
	case HARDWARE_BACK_CAM_GET:
		hardwareinfo_num = HARDWARE_BACK_CAM;
		break;
	case HARDWARE_BACK_SUBCAM_GET:
		hardwareinfo_num = HARDWARE_BACK_SUBCAM;
		break;
	case HARDWARE_MACRO_CAM_GET:
		hardwareinfo_num = HARDWARE_MACRO_CAM;
		break;
	case HARDWARE_BT_GET:
		hardwareinfo_set_prop(HARDWARE_BT, "SC9863A1");
		hardwareinfo_num = HARDWARE_BT;
		break;
	case HARDWARE_WIFI_GET:
		hardwareinfo_set_prop(HARDWARE_WIFI, "SC9863A1");
		hardwareinfo_num = HARDWARE_WIFI;
		break;
	case HARDWARE_ACCELEROMETER_GET:
		hardwareinfo_num = HARDWARE_ACCELEROMETER;
		break;
	case HARDWARE_ALSPS_GET:
		hardwareinfo_num = HARDWARE_ALSPS;
		break;
	case HARDWARE_GYROSCOPE_GET:
		hardwareinfo_num = HARDWARE_GYROSCOPE;
		break;
	case HARDWARE_MAGNETOMETER_GET:
		hardwareinfo_num = HARDWARE_MAGNETOMETER;
		break;
	case HARDWARE_SAR_GET:
		//hardwareinfo_set_prop(HARDWARE_SAR, Sar_name);
		hardwareinfo_num = HARDWARE_SAR;
		break;
	case HARDWARE_GPS_GET:
		hardwareinfo_set_prop(HARDWARE_GPS, "SC9863A1");
	    hardwareinfo_num = HARDWARE_GPS;
		break;
	case HARDWARE_FM_GET:
		hardwareinfo_set_prop(HARDWARE_FM, "SC9863A1");
	    hardwareinfo_num = HARDWARE_FM;
		break;
	case HARDWARE_NFC_GET:
		hardwareinfo_set_prop(HARDWARE_NFC, "PN557");
		hardwareinfo_num = HARDWARE_NFC;
		break;
	case HARDWARE_BATTERY_ID_GET:
		hardwareinfo_num = HARDWARE_BATTERY_ID;
		break;
	case HARDWARE_CHARGER_IC_INFO_GET:
		hardwareinfo_num = HARDWARE_CHARGER_IC_INFO;
		break;
	case HARDWARE_BMS_GAUGE_GET:
		hardwareinfo_num = HARDWARE_BMS_GAUGE;
		break;
	case HARDWARE_BACK_CAM_MOUDULE_ID_GET:
		hardwareinfo_num = HARDWARE_BACK_CAM_MOUDULE_ID;
		break;
	case HARDWARE_BACK_SUBCAM_MODULEID_GET:
		hardwareinfo_num = HARDWARE_BACK_SUBCAM_MOUDULE_ID;
		break;
	case HARDWARE_FRONT_CAM_MODULE_ID_GET:
		hardwareinfo_num = HARDWARE_FRONT_CAM_MOUDULE_ID;
		break;
	case HARDWARE_MACRO_CAM_MOUDULE_ID_GET:
		hardwareinfo_num = HARDWARE_MACRO_CAM_MOUDULE_ID;
		break;
	case HARDWARE_BOARD_ID_GET:
		boardid_get();
		hardwareinfo_set_prop(HARDWARE_BOARD_ID, board_id);
		hardwareinfo_num = HARDWARE_BOARD_ID;
		break;

        case HARDWARE_SMARTPA_IC_INFO_GET:
		hardwareinfo_num = HARDWARE_SMARTPA_IC_INFO;
		break;

	case HARDWARE_HARDWARE_ID_GET:
		hwid_get();
		hardwareinfo_set_prop(HARDWARE_HARDWARE_ID, hardware_id);
		hardwareinfo_num = HARDWARE_HARDWARE_ID;
		break;
	//Modify by huangmiao@wingtech.com for SCT-702  add hardware info for factory on start(2021.9.9)
	case HARDWARE_CAMERA_INFO_SET:
	{
		camera_info hardware;
		memset(&hardware, 0, sizeof(camera_info));
		if(copy_from_user(&hardware, data, sizeof(camera_info)))
		{
			pr_err("copy_from_user HARDWARE_CAMERA_INFO_SET error");
			ret =  -EINVAL;
		}
		else
		{
			strncpy(hardwareinfo_name[hardware.param], hardware.str, hardware.lenth);
			pr_info("copy_from_user HARDWARE_CAMERA_INFO_SET %d, %s", hardware.param, hardwareinfo_name[hardware.param]);
		}
		goto set_ok;
		break;
	}
	//Modify by huangmiao@wingtech.com for SCT-702  add hardware info for factory on end(2021.9.9)
	default:
		ret = -EINVAL;
		goto err_out;
	}
	//memset(data, 0, HARDWARE_MAX_ITEM_LONGTH);//clear the buffer
	if (copy_to_user(data, hardwareinfo_name[hardwareinfo_num], strlen(hardwareinfo_name[hardwareinfo_num]))){
		//printk("%s, copy to usr error\n", __func__);
		ret =  -EINVAL;
	}
set_ok:
err_out:
	return ret;
}

static ssize_t show_boardinfo(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i = 0;
	char temp_buffer[HARDWARE_MAX_ITEM_LONGTH];
	int buf_size = 0;

	for(i = 0; i < HARDWARE_MAX_ITEM; i++)
	{
		memset(temp_buffer, 0, HARDWARE_MAX_ITEM_LONGTH);
		if(i == HARDWARE_LCD)
		{
			sprintf(temp_buffer, "%s : %s\n", hardwareinfo_items[i], Lcm_name);
		}
		else if(i == HARDWARE_BT || i == HARDWARE_WIFI || i == HARDWARE_GPS || i == HARDWARE_FM)
		{
			sprintf(temp_buffer, "%s : %s\n", hardwareinfo_items[i], "Qualcomm");
		}
		else
		{
			sprintf(temp_buffer, "%s : %s\n", hardwareinfo_items[i], hardwareinfo_name[i]);
		}
		strcat(buf, temp_buffer);
		buf_size +=strlen(temp_buffer);
	}

	return buf_size;
}
static DEVICE_ATTR(boardinfo, S_IRUGO, show_boardinfo, NULL);


static int boardinfo_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int rc = 0;

	printk("%s: start\n", __func__);

	rc = device_create_file(dev, &dev_attr_boardinfo);
	if (rc < 0)
		return rc;

	dev_info(dev, "%s: ok\n", __func__);

	return 0;
}

static int boardinfo_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	device_remove_file(dev, &dev_attr_boardinfo);
	dev_info(&pdev->dev, "%s\n", __func__);
	return 0;
}


static struct file_operations hardwareinfo_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.unlocked_ioctl = hardwareinfo_ioctl,
	.compat_ioctl = hardwareinfo_ioctl,
};

static struct miscdevice hardwareinfo_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "hardwareinfo",
	.fops = &hardwareinfo_fops,
};

#if 0
static struct of_device_id boardinfo_of_match[] = {
	{ .compatible = "wt:boardinfo", },
	{}
};
#endif

static struct platform_driver boardinfo_driver = {
	.driver = {
		.name	= "boardinfo",
		.owner	= THIS_MODULE,
		//.of_match_table = boardinfo_of_match,
	},
	.probe		= boardinfo_probe,
	.remove		= boardinfo_remove,
};



static int __init hardwareinfo_init_module(void)
{
	int ret, i;

	for(i = 0; i < HARDWARE_MAX_ITEM; i++)
		strcpy(hardwareinfo_name[i], "NULL");

	ret = misc_register(&hardwareinfo_device);
	if(ret < 0){
		return -ENODEV;
	}

	ret = platform_driver_register(&boardinfo_driver);
	if(ret != 0)
	{
		return -ENODEV;
	}

	lcd_name_get();

	return 0;
}

static void __exit hardwareinfo_exit_module(void)
{
	misc_deregister(&hardwareinfo_device);
}

module_init(hardwareinfo_init_module);
module_exit(hardwareinfo_exit_module);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ming He <heming@wingtech.com>");
//Added by wt.yanrenjie for SCT-702,wt hardware info end



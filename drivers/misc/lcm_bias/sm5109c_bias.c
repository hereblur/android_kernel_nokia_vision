#include <drm/drm_panel.h>

#include "sm5109c_bias.h"

static struct i2c_client *sm5109c_i2c_client;
static struct mutex sm5109_rw_lock;

static uint8_t sm5109_reg0[] = {0x00,0x14};
static uint8_t sm5109_reg1[] = {0x01,0x14 };

static int i2c_sm5109c_write(struct i2c_client *client, uint8_t *tx_data, uint8_t length)
{
    int ret;

    struct i2c_msg msg[] =
    {
        {
            .addr = client->addr,
            .flags = 0,
            .len = length,
            .buf = tx_data,
        }
    };

    ret = i2c_transfer(client->adapter, msg, 1);

    if (ret != 1)
        printk("%s: i2c_write err: ret = %d\n", __func__, ret);

    return ret;
}

static int sm5109c_write_bias_6v(void)
{
	int ret;

	pr_info("sm5109c: %s\n", __func__);
	ret = i2c_sm5109c_write(sm5109c_i2c_client, sm5109_reg0, 2);
	ret = i2c_sm5109c_write(sm5109c_i2c_client, sm5109_reg1, 2);
	if(ret < 0) {
			printk("sm5109c write regster failed ! \n");
	}

	return ret;
}

#if defined(CONFIG_DRM_PANEL)
static struct drm_panel *active_panel;

static int sm5109c_notifier_callback(struct notifier_block *self,
		unsigned long event, void *data)
{
	struct drm_panel_notifier *evdata = data;
	int *blank;

	if (!evdata)
		return 0;

	if (evdata->data && event == DRM_PANEL_EVENT_BLANK) {
		blank = evdata->data;
		switch (*blank) {
		case DRM_PANEL_BLANK_POWERDOWN:
//			pr_info("%s: event DRM_PANEL_BLANK_POWERDOWN\n", __func__);
			break;
		case DRM_PANEL_BLANK_UNBLANK:
//			pr_info("%s: event DRM_PANEL_BLANK_UNBLANK\n", __func__);
//			sm5109c_write_bias_6v();
			break;
		}
	}

	if (evdata->data && event == DRM_PANEL_EARLY_EVENT_BLANK) {
		blank = evdata->data;
		switch (*blank) {
		case 0x10:
			pr_info("%s: event DRM_PANEL_EARLY_BLANK_UNBLANK\n", __func__);
			sm5109c_write_bias_6v();
			break;
		}
	}

	return 0;
}

static struct notifier_block sm5109c_notifier_block = {
	.notifier_call = sm5109c_notifier_callback,
};

#endif

static int sm5109c_parse_dt(struct device *dev)
{
    struct device_node *np;
#if defined(CONFIG_DRM_PANEL)
	struct device_node *node = NULL;
	struct device_node *child, *lcds_node;
	struct drm_panel *panel = NULL;
#endif
//  int ret = 0;

    if (!dev)
        return -1;

    np = dev->of_node;

#if defined(CONFIG_DRM_PANEL)
	pr_info("%s: to find phandle 'panel'\n", __func__);

	lcds_node = of_find_node_by_path("/lcds");
	for_each_child_of_node(lcds_node, child) {
		pr_info("%s: lcds child %s\n", __func__, child->name);
		panel = of_drm_find_panel(child);
		if (!IS_ERR(panel)) {
			active_panel = panel;
			pr_info("%s:%d active_panel=%p\n", __func__, __LINE__, active_panel);
			return 0;
		} else {
			pr_info("%s is not ready , %d\n", __func__, PTR_ERR(panel));
		}
	}

	node = of_parse_phandle(np, "panel", 0);
	if(node) {
		pr_info("%s: find phandle panel %s \n", __func__, node->name);
		panel = of_drm_find_panel(node);
		of_node_put(node);
		if (!IS_ERR(panel)) {
			pr_info("%s find drm_panel successfully\n", __func__);
			active_panel = panel;
			pr_info("%s:%d active_panel=%p\n", __func__, __LINE__, active_panel);
			return 0;
		} else {
			pr_info("%s is not ready , %d\n", __func__, PTR_ERR(panel));
		}
	}
#endif

    return 0;
}

static ssize_t bias_suspend_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{

	return 0;
}

static ssize_t bias_suspend_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	static bias_state = 0;
	
	pr_info("sm5109c: %s\n", __func__);
  
	if (buf[0] == '0' && bias_state == 0) {
		bias_state = 1;
		sm5109c_write_bias_6v();
	}
	else if (buf[0] == '1')
		bias_state = 0;
	return count;
}

static DEVICE_ATTR_RW(bias_suspend);

static struct attribute *sm5109c_atts[] = {
	&dev_attr_bias_suspend.attr,
	NULL,
};
//ATTRIBUTE_GROUPS(sm5109c);

static const struct attribute_group sm5109c_atts_group = {
	.attrs = sm5109c_atts,
};

static int sm5109c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;

	printk("lcd bias ic addr:0x%02x",client->addr);

	sm5109c_i2c_client = client;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk("I2C check functionality failed.");
		return -1;
	}

	sm5109c_parse_dt(&client->dev);
	mutex_init(&sm5109_rw_lock);

	ret = sysfs_create_group(&(client->dev.kobj), &sm5109c_atts_group);
	if(ret)
		pr_err("%s: create panel attr node failed, rc=%d.\n", __func__, ret);
//	if (sysfs_create_link(NULL, &(client->dev.kobj), "panel_bias") < 0)
//		pr_err("%s: Failed to create touchscreen link!\n",__func__);

#if defined(CONFIG_DRM_PANEL)
	pr_info("%s: drm_panel_notifier_register\n", __func__);
	if (active_panel) {
		ret = drm_panel_notifier_register(active_panel,	&sm5109c_notifier_block);
		pr_info("%s: drm_panel_notifier_register %d\n", __func__, ret);
	}
#endif

	sm5109c_write_bias_6v();

	pr_info("%s: lcd bias probe end \n", __func__);

	return 0;
}

static int sm5109c_remove(struct i2c_client *client)
{
	printk("sm5109c driver removing...");

//	sysfs_remove_link(NULL, "panel_bias");
	sysfs_remove_group(&(client->dev.kobj), &sm5109c_atts_group);

   return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id sm5109c_match_table[] = {
	{.compatible = "silicon,sm5109c",},
	{ },
};
#endif

static const struct i2c_device_id sm5109c_id[] = {
	{SILICON_I2C_NAME, 0},
	{}
};

static struct i2c_driver sm5109c_driver = {
	.probe = sm5109c_probe,
	.remove = sm5109c_remove,
	.id_table = sm5109c_id,
	.driver = {
	.name = SILICON_I2C_NAME,
	.owner = THIS_MODULE,
#ifdef CONFIG_OF
	.of_match_table = sm5109c_match_table,
#endif
	},
};

static int __init sm5109c_init(void)
{
	printk("sm5109c_init driver installing...");
	return i2c_add_driver(&sm5109c_driver);
}

static void __exit sm5109c_exit(void)
{
	printk ("sm5109c driver exited.");
	i2c_del_driver(&sm5109c_driver);
}

module_init(sm5109c_init);
module_exit(sm5109c_exit);

MODULE_DESCRIPTION("Lcm Bias Driver");
MODULE_LICENSE("GPL");

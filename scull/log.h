#ifndef __LOG_H
#define __LOG_H

#ifdef DEBUG
#define DEBUG_LOG(fmt,arg...) printk(KERN_DEBUG"(%s:%d): "fmt,__func__, __LINE__, ##arg)
#else
#define DEBUG_LOG(fmt, arg...)
#endif
#define NORM_LOG(fmt, arg...) printk(KERN_NOTICE"(%s:%d): "fmt, __func__, __LINE__, ##arg)
#define WARNING_LOG(fmt, arg...) printk(KERN_WARNING"(%s:%d): "fmt, __func__, __LINE__, ##arg)
#define ERR_LOG(fmt, arg...) printk(KERN_ERR"(%s:%d): "fmt, __func__, __LINE__, ##arg)

#endif
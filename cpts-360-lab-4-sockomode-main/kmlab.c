#define LINUX

#include "kmlab_given.h"

#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/list.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lin");
MODULE_DESCRIPTION("CPTS360 Lab 4");

#define DEBUG 1

typedef struct {
    struct list_head list;
    unsigned int pid;
    unsigned long cpu_time;
} proc_list;

static struct proc_dir_entry *proc_dir, *proc_entry;
LIST_HEAD(kmlab_proc_list);
static struct timer_list kmlab_timer;
static struct workqueue_struct *kmlab_workqueue;
static spinlock_t kmlab_lock;
static struct work_struct *kmlab_work;

unsigned long cputime64_to_jiffies_convert(u64 cputime) //converts nanoseconds to jiffies
{

    return (unsigned long)(cputime / (NSEC_PER_SEC / 1000));
}

static ssize_t kmlab_read(struct file *file, char __user *buffer, size_t count, loff_t *data) //read function
{
    unsigned long copied = 0;
    char *buf;
    proc_list *tmp;
    unsigned long flags;

    buf = (char *)kmalloc(count, GFP_KERNEL);

    spin_lock_irqsave(&kmlab_lock, flags); //enters critical section
    list_for_each_entry(tmp, &kmlab_proc_list, list) 
    {
      copied += snprintf(buf + copied, count - copied, "%u: %u\n", tmp->pid, jiffies_to_msecs(cputime64_to_jiffies_convert(tmp->cpu_time)));

    }
    spin_unlock_irqrestore(&kmlab_lock, flags);

    buf[copied] = '\0';

    if (copy_to_user(buffer, buf, copied)) // copy buffer to user space
   {
      // just to clear compiler warning
   }

    kfree(buf);

    return copied;
}

static ssize_t kmlab_write(struct file *file, const char __user *buffer, size_t count, loff_t *data) // write function
{
    char *buf;
    unsigned int pid;
    proc_list *tmp;
    unsigned long flags;

    buf = (char *)kmalloc(count + 1, GFP_KERNEL);
    if (!buf)
        return -ENOMEM;

    if (copy_from_user(buf, buffer, count)) // data gets copied from user
    {
        kfree(buf);
        return -EFAULT;
    }

    buf[count] = '\0';

    if (sscanf(buf, "%u", &pid) != 1) // parse input and extract PID
    {
        kfree(buf);
        return -EINVAL;  // returns if invalid argument
    }

    tmp = (proc_list *)kmalloc(sizeof(proc_list), GFP_KERNEL); // allocates memory for new process
    if (!tmp) 
    {
        kfree(buf);
        return -ENOMEM;
    }

    tmp->pid = pid; // for initalizing the new PID processes
    tmp->cpu_time = 0;

    spin_lock_irqsave(&kmlab_lock, flags);
    list_add(&(tmp->list), &kmlab_proc_list); // adds new process to kmlab_proc_list
    spin_unlock_irqrestore(&kmlab_lock, flags);

    kfree(buf);

    return count;
}

static const struct proc_ops kmlab_fops = {
    .proc_read = kmlab_read,
    .proc_write = kmlab_write,
};

void kmlab_timer_callback(struct timer_list *timer) // callback function
{
    queue_work(kmlab_workqueue, kmlab_work); // work queue for background processing
}

static void kmlab_work_function(struct work_struct *work) 
{
    unsigned long flags;
    proc_list *pos, *n;

    spin_lock_irqsave(&kmlab_lock, flags);
    list_for_each_entry_safe(pos, n, &kmlab_proc_list, list) 
    {
        if (get_cpu_use(pos->pid, &pos->cpu_time) == -1) 
        {
            list_del(&pos->list);
            kfree(pos);
        }
    }
    spin_unlock_irqrestore(&kmlab_lock, flags);

    mod_timer(&kmlab_timer, jiffies + msecs_to_jiffies(5000));
}

static int __init kmlab_init(void) //function for loading in modules
{
    #ifdef DEBUG
    printk(KERN_ALERT "Module loading.\n");
    #endif

    
    if ((proc_dir = proc_mkdir("kmlab", NULL)) == NULL) // creates /proc/kmlab
    {
        printk(KERN_INFO "ERROR: proc_mkdir unsuccessful\n"); // returns error
        return -ENOMEM;
    }
    
    if ((proc_entry = proc_create("status", 0666, proc_dir, &kmlab_fops)) == NULL)  // creates status
    {
        remove_proc_entry("kmlab", NULL);
        printk(KERN_INFO "ERROR: proc_create unsuccessful\n"); // returns error 
        return -ENOMEM;
    }

    timer_setup(&kmlab_timer, kmlab_timer_callback, 0);
    mod_timer(&kmlab_timer, jiffies + msecs_to_jiffies(5000));


    if ((kmlab_workqueue = create_workqueue("kmlab_workqueue")) == NULL) 
    {
        printk(KERN_INFO "ERROR: Failed to create workqueue\n"); //return errror if workqueue can't be created
        return -ENOMEM;
    }

    kmlab_work = (struct work_struct *)kmalloc(sizeof(struct work_struct), GFP_KERNEL); // initalizes work
    INIT_WORK(kmlab_work, kmlab_work_function);

    spin_lock_init(&kmlab_lock); // initializes work 

    printk(KERN_ALERT "Module successfully loaded.\n"); // returns successful module load
    return 0;
}

static void __exit kmlab_exit(void) // function for unloading modules
{
    proc_list *pos, *n;

    #ifdef DEBUG
    printk(KERN_ALERT "Unloading module.\n");
    #endif

    remove_proc_entry("status", proc_dir);
    remove_proc_entry("kmlab", NULL);

    del_timer_sync(&kmlab_timer);

    list_for_each_entry_safe(pos, n, &kmlab_proc_list, list) 
    {
        list_del(&pos->list);
        kfree(pos);
    }

    flush_workqueue(kmlab_workqueue); // frees workqueue
    destroy_workqueue(kmlab_workqueue); //destroys workqueue

    kfree(kmlab_work); // frees work 

    printk(KERN_ALERT "Module successfully unloaded.\n");
}

module_init(kmlab_init); //init function
module_exit(kmlab_exit); //exit function
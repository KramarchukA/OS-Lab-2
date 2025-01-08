#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/time.h>
#include <linux/ktime.h>
#include <linux/types.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#define procfs_name "tsulab"
#define SPEED_OF_LIGHT 299792 // km/sec
#define EARTH_EQUATOR_LENGTH 40075 //km
static struct proc_dir_entry *our_proc_file = NULL;

//количество оборотов света вокруг экватора Земли до следующего нового года
static int calculate_revs_till_new_year(void) {
	struct tm tm;
	time64_t current_time, new_year_time;
	long long secs_remaining;
    long long number_of_revolutions;

	current_time = ktime_get_real_seconds();
	time64_to_tm(current_time, 0, &tm);

	new_year_time = mktime64(tm.tm_year + 1900 + 1, 1, 1, 0, 0, 0);

	secs_remaining = (new_year_time - current_time);
    int revs_per_sec = SPEED_OF_LIGHT / EARTH_EQUATOR_LENGTH;
    number_of_revolutions = revs_per_sec * secs_remaining;
	return number_of_revolutions;
}


static ssize_t procfile_read(struct file *file_pointer, char __user *buffer, size_t buffer_length, loff_t *offset) {
	int revolutions = calculate_revs_till_new_year();

    if (*offset > 0) {
        return 0; 
    }

    char s[20]; 
    int len = sprintf(s, "%d\n", revolutions);
    copy_to_user(buffer, s, len);
        
    *offset += len; 
    return len; 
}

static const struct proc_ops proc_file_fops = {
	.proc_read = procfile_read,
};

static int __init procfs1_init(void) {
	our_proc_file = proc_create(procfs_name, 0644, NULL, &proc_file_fops);
	return 0;
}

static void __exit procfs1_exit(void) {
	proc_remove(our_proc_file);
}

module_init(procfs1_init);
module_exit(procfs1_exit);

MODULE_LICENSE("GPL");
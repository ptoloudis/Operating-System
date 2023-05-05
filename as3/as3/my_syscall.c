// slob_get_total_alloc_mem
// slob_get_total_free_mem

#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/slab.h>

SYSCALL_DEFINE0(slob_get_total_alloc_mem)
{
  return alloc_mem;
}

SYSCALL_DEFINE0(slob_get_total_free_mem)
{
  return free_mem;
}

SYSCALL_DEFINE1( my_kmalloc, int, size)
{
  void* ptr = kmalloc(size, GFP_KERNEL);
  if (ptr == NULL) {
    printk("kmalloc failed\n");
    return (long int)-ENOMEM;
  } else {
    return (long int)ptr;
  }
}

SYSCALL_DEFINE1(my_kfree, void *, ptr)
{
  kfree(ptr);
  return 0;
}
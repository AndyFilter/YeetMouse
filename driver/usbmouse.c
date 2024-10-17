#include "accel.h"
#include "config.h"
#include "util.h"

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/usb/input.h>
#include <linux/hid.h>
#include <linux/version.h>

#define NONE_EVENT_VALUE 0

/* Applies new values to events, filtering out zeroed values */
INLINE unsigned int usb_mouse_update_events(
  struct input_value* vals,
  unsigned int count,
  int x,
  int y,
  int wheel
) {
  struct input_value *end = vals;
  struct input_value *v;
  for (v = vals; v != vals + count; v++) {
    if (v->type == EV_REL) {
      switch (v->code) {
      case REL_X:
        if (x == NONE_EVENT_VALUE)
          continue;
        v->value = x;
        break;
      case REL_Y:
        if (y == NONE_EVENT_VALUE)
          continue;
        v->value = y;
        break;
      case REL_WHEEL:
        if (wheel == NONE_EVENT_VALUE)
          continue;
        v->value = wheel;
        break;
      }
    }
    if (end != v)
      *end = *v;
    end++;
  }
  return end - vals;
}

static unsigned int usb_mouse_events(struct input_handle *handle, struct input_value *vals, unsigned int count) {
  struct input_dev *dev = handle->dev;
  unsigned int out_count = count;
  struct input_value *end = vals;
  struct input_value *v;

  struct input_value *v_x = NULL;
  struct input_value *v_y = NULL;
  struct input_value *v_wheel = NULL;
  struct input_value *v_syn = NULL;

  /* Find input_value for EV_REL events we're interested in and store pointers */
  for (v = vals; v != vals + count; v++) {
    if (v->type == EV_REL) {
      switch (v->code) {
      case REL_X:
        v_x = v;
        break;
      case REL_Y:
        v_y = v;
        break;
      case REL_WHEEL:
        v_wheel = v;
        break;
      }
      /* TODO: What if we get duplicate events before a SYN?
       * It should, in theory, be possible to protect ourselves from this.
       * However, since `accelerate` is stateful, that's currently not a good idea. */
    }
  }

  if (v_x != NULL || v_y != NULL || v_wheel != NULL) {
    /* Retrieve values if any EV_REL events were found */
    int x = NONE_EVENT_VALUE;
    int y = NONE_EVENT_VALUE;
    int wheel = NONE_EVENT_VALUE;
    if (v_x != NULL)
      x = v_x->value;
    if (v_y != NULL)
      y = v_y->value;
    if (v_wheel != NULL)
      wheel = v_wheel->value;
    /* Attempt to apply acceleration */
    if (!accelerate(&x, &y, &wheel)) {
      out_count = usb_mouse_update_events(vals, count, x, y, wheel);
      /* Apply new values to the queued (raw) events, same as above.
       * NOTE: This might (strictly speaking) not be necessary, but this way we leave
       * no trace of the unmodified values, in case another subsystem uses them. */
      dev->num_vals = usb_mouse_update_events(dev->vals, dev->num_vals, x, y, wheel);
    }
  }

  return out_count;
}

static bool usb_mouse_match(struct input_handler *handler, struct input_dev *dev) {
  if (!dev->dev.parent)
    return false;
  struct hid_device *hdev = to_hid_device(dev->dev.parent);
  printk("LEETMOUSE: found a possible mouse %s", hdev->name);
  return hdev->type == HID_TYPE_USBMOUSE;
}

/* Same as Linux's input_register_handle but we always add the handle to the head of handlers */
int usb_mouse_register_handle_head(struct input_handle *handle) {
  struct input_handler *handler = handle->handler;
  struct input_dev *dev = handle->dev;
  int error = mutex_lock_interruptible(&dev->mutex);
  if (error)
    return error;
  list_add_rcu(&handle->d_node, &dev->h_list);
  mutex_unlock(&dev->mutex);
  list_add_tail_rcu(&handle->h_node, &handler->h_list);
  if (handler->start)
    handler->start(handle);
  return 0;
}

static int usb_mouse_connect(struct input_handler *handler, struct input_dev *dev, const struct input_device_id *id) {
  struct input_handle *handle;
  int error;

  handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
  if (!handle)
    return -ENOMEM;

  handle->dev = input_get_device(dev);
  handle->handler = handler;
  handle->name = "yeetmouse";

  /* WARN: Instead of `input_register_handle` we use a customized version of it here.
   * This prepends the handler (like a filter) instead of appending it, making
   * it take precedence over any other input handler that'll be added. */
  error = usb_mouse_register_handle_head(handle);
  if (error)
    goto err_free_mem;

  error = input_open_device(handle);
  if (error)
    goto err_unregister_handle;

  printk(pr_fmt("yeetmouse: connecting to device: %s (%s at %s)"), dev_name(&dev->dev), dev->name ?: "unknown", dev->phys ?: "unknown");

  return 0;

err_unregister_handle:
  input_unregister_handle(handle);

err_free_mem:
  kfree(handle);
  return error;
}

static void usb_mouse_disconnect(struct input_handle *handle) {
  input_close_device(handle);
  input_unregister_handle(handle);
  kfree(handle);
}

static const struct input_device_id usb_mouse_ids[] = {
  {
    .flags = INPUT_DEVICE_ID_MATCH_EVBIT,
    .evbit = { BIT_MASK(EV_REL) }
  },
  {},
};

MODULE_DEVICE_TABLE(input, usb_mouse_ids);

struct input_handler usb_mouse_handler = {
  .name = "yeetmouse",
  .id_table = usb_mouse_ids,
  .events = usb_mouse_events,
  .connect = usb_mouse_connect,
  .disconnect = usb_mouse_disconnect,
  .match = usb_mouse_match
};

static int __init usb_mouse_init(void) {
  return input_register_handler(&usb_mouse_handler);
}

static void __exit usb_mouse_exit(void) {
  input_unregister_handler(&usb_mouse_handler);
}

MODULE_DESCRIPTION("USB HID input handler applying mouse acceleration (Yeetmouse)");
MODULE_LICENSE("GPL");

module_init(usb_mouse_init);
module_exit(usb_mouse_exit);

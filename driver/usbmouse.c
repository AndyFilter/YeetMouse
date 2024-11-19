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

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0))
  #define __cleanup_events 0
#else
  #define __cleanup_events 1
#endif

struct usb_mouse_state {
  int x;
  int y;
  int wheel;
};

#if __cleanup_events
  static unsigned int usb_mouse_events(struct input_handle *handle, struct input_value *vals, unsigned int count) {
#else
  static void usb_mouse_events(struct input_handle *handle, const struct input_value *vals, unsigned int count) {
#endif
  struct usb_mouse_state *state = handle->private;
  struct input_dev *dev = handle->dev;
  unsigned int out_count = count;
  struct input_value *v_syn = NULL;
  struct input_value *end = vals;
  struct input_value *v;
  int error;

  for (v = vals; v != vals + count; v++) {
    if (v->type == EV_REL) {
      /* Find input_value for EV_REL events we're interested in and store values */
      switch (v->code) {
      case REL_X:
        state->x = v->value;
        break;
      case REL_Y:
        state->y = v->value;
        break;
      case REL_WHEEL:
        state->wheel = v->value;
        break;
      }
    } else if (
      (state->x != NONE_EVENT_VALUE || state->y != NONE_EVENT_VALUE) &&
        v->type == EV_SYN && v->code == SYN_REPORT
    ) {
      /* If we find an EV_SYN event, and we've seen x/y values, we store the pointer and apply acceleration next */
      v_syn = v;
      break;
    }
  }

  if (v_syn != NULL) {
    /* Retrieve to state if an EV_SYN event was found and apply acceleration */
    int x = state->x;
    int y = state->y;
    int wheel = state->wheel;
    /* If we found no values to update, return */
    if (x == NONE_EVENT_VALUE && y == NONE_EVENT_VALUE && wheel == NONE_EVENT_VALUE)
      goto unchanged_return;
    error = accelerate(&x, &y, &wheel);
    /* Reset state */
    state->x = NONE_EVENT_VALUE;
    state->y = NONE_EVENT_VALUE;
    state->wheel = NONE_EVENT_VALUE;
    /* Deal with left over EV_REL events we should take into account for the next run */
    for (v = v_syn; v != vals + count; v++) {
      if (v->type == EV_REL) {
        /* Store values for next runthrough */
        switch (v->code) {
        case REL_X:
          state->x = v->value;
          break;
        case REL_Y:
          state->y = v->value;
          break;
        case REL_WHEEL:
          state->wheel = v->value;
          break;
        }
      }
    }
    /* Apply updates after we've captured events for next run */
    if (!error) {
      for (v = vals; v != vals + count; v++) {
        if (v->type == EV_REL) {
          switch (v->code) {
          case REL_X:
            if (__cleanup_events && x == NONE_EVENT_VALUE)
              continue;
            v->value = x;
            break;
          case REL_Y:
            if (__cleanup_events && y == NONE_EVENT_VALUE)
              continue;
            v->value = y;
            break;
          case REL_WHEEL:
            if (__cleanup_events && wheel == NONE_EVENT_VALUE)
              continue;
            v->value = wheel;
            break;
          }
        }
        if (end != v)
          *end = *v;
        end++;
      }
      out_count = end - vals;
      /* Apply new values to the queued (raw) events, same as above.
       * NOTE: This might (strictly speaking) not be necessary, but this way we leave
       * no trace of the unmodified values, in case another subsystem uses them. */
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
      dev->num_vals = end - vals;
    }
  }
  /* NOTE: Technically, we can also stop iterating over `vals` when we find EV_SYN, apply acceleration,
   * then restart in a loop until we reach the end of `vals` to handle multiple EV_SYN events per batch.
   * However, that's not necessary since we can assume that all events in `vals` apply to the same moment
   * in time. */
unchanged_return:
#if __cleanup_events
  return out_count;
#endif
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

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 11, 7))
  /* In 6.11.7 an additional handler pointer was added: https://github.com/torvalds/linux/commit/071b24b54d2d05fbf39ddbb27dee08abd1d713f3 */
  if (handler->events)
    handle->handle_events = handler->events;
#endif

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
  struct usb_mouse_state *state;
  int error;

  handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
  if (!handle)
    return -ENOMEM;

  state = kzalloc(sizeof(struct usb_mouse_state), GFP_KERNEL);
  if (!state) {
    kfree(handle);
    return -ENOMEM;
  }

  state->x = NONE_EVENT_VALUE;
  state->y = NONE_EVENT_VALUE;
  state->wheel = NONE_EVENT_VALUE;

  handle->private = state;
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
  kfree(handle->private);
  kfree(handle);
  return error;
}

static void usb_mouse_disconnect(struct input_handle *handle) {
  input_close_device(handle);
  input_unregister_handle(handle);
  kfree(handle->private);
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

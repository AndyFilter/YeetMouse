// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 1999-2001 Vojtech Pavlik, USB HIDBP Mouse support
 * Copyright (c) Gnarus-G, maccel
 * This file has an input handler derived from the maccel project
 * https://github.com/Gnarus-G/maccel/blob/dedaa97/driver/input_handler.h
 */

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

#define DRIVER_VERSION "v1.6"
#define DRIVER_AUTHOR "Vojtech Pavlik <vojtech@ucw.cz>"
#define DRIVER_DESC "USB HID mouse driver with acceleration (LEETMOUSE)"

#define NONE_EVENT_VALUE 0
static int relative_axis_events[REL_CNT] = {
  /* [x, y, ..., wheel, ...] */ NONE_EVENT_VALUE
};

struct input_dev *virtual_input_dev;

// The virtual_input_dev is declared for/in the maccel_input_handler module.
// This initializes it.
static int create_virtual_device(void) {
  int error;

  virtual_input_dev = input_allocate_device();
  if (!virtual_input_dev) {
    printk(KERN_ERR "Failed to allocate virtual input device\n");
    return -ENOMEM;
  }

  virtual_input_dev->name = "Yeetmouse";
  virtual_input_dev->id.bustype = BUS_USB;
  virtual_input_dev->id.version = 1;

  // Set the supported event types and codes for the virtual device
  // and for some reason not setting some EV_KEY bits causes a noticeable
  // difference in the values we operate on, leading to a different
  // acceleration behavior than we expect.
  set_bit(EV_KEY, virtual_input_dev->evbit);
  set_bit(BTN_LEFT, virtual_input_dev->keybit);

  set_bit(EV_REL, virtual_input_dev->evbit);
  for (u32 code = REL_X; code < REL_CNT; code++) {
    set_bit(code, virtual_input_dev->relbit);
  }

  error = input_register_device(virtual_input_dev);
  if (error) {
    printk(KERN_ERR "LEETMOUSE: Failed to register virtual input device\n");
    input_free_device(virtual_input_dev);
    return error;
  }

  return 0;
}

static bool usb_mouse_filter(struct input_handle *handle, u32 type, u32 code, int value) {
  switch (type) {
    case EV_REL: {
      printk("LEETMOUSE: EV_REL => code %d, value %d", code, value);
      relative_axis_events[code] = value;
      return true; // skip unaccelerated mouse input.
    }
    case EV_SYN: {
      int x = relative_axis_events[REL_X];
      int y = relative_axis_events[REL_Y];
      int wheel = relative_axis_events[REL_WHEEL];
      if (x || y || wheel) {
        printk("LEETMOUSE: EV_SYN => code %d", code);

        if (!accelerate(&x, &y, &wheel)) {
          if (x) {
            input_report_rel(virtual_input_dev, REL_X, x);
          }
          if (y) {
            input_report_rel(virtual_input_dev, REL_Y, y);
          }
          if (wheel) {
            input_report_rel(virtual_input_dev, REL_WHEEL, wheel);
          }
        }

        relative_axis_events[REL_X] = NONE_EVENT_VALUE;
        relative_axis_events[REL_Y] = NONE_EVENT_VALUE;
        relative_axis_events[REL_WHEEL] = NONE_EVENT_VALUE;
      }

      for (u32 code = REL_Z; code < REL_CNT; code++) {
        int value = relative_axis_events[code];
        if (value != NONE_EVENT_VALUE) {
          input_report_rel(virtual_input_dev, code, value);
          relative_axis_events[code] = NONE_EVENT_VALUE;
        }
      }

      input_sync(virtual_input_dev);

      return false;
    }

    default:
      return false;
  }
}

static bool usb_mouse_match(struct input_handler *handler, struct input_dev *dev) {
  if (dev == virtual_input_dev || !dev->dev.parent)
    return false;
  struct hid_device *hdev = to_hid_device(dev->dev.parent);
  printk("LEETMOUSE: found a possible mouse %s", hdev->name);
  return hdev->type == HID_TYPE_USBMOUSE;
}

static int usb_mouse_connect(struct input_handler *handler, struct input_dev *dev, const struct input_device_id *id) {
  struct input_handle *handle;
  int error;

  handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
  if (!handle)
    return -ENOMEM;

  handle->dev = input_get_device(dev);
  handle->handler = handler;
  handle->name = "leetmouse";

  error = input_register_handle(handle);
  if (error)
    goto err_free_mem;

  error = input_open_device(handle);
  if (error)
    goto err_unregister_handle;

  printk(pr_fmt("LEETMOUSE: connecting to device: %s (%s at %s)"), dev_name(&dev->dev), dev->name ?: "unknown", dev->phys ?: "unknown");

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
  .name = "leetmouse",
  .id_table = usb_mouse_ids,
  .filter = usb_mouse_filter,
  .connect = usb_mouse_connect,
  .disconnect = usb_mouse_disconnect,
  .match = usb_mouse_match
};

static int __init usb_mouse_init(void) {
  int error = create_virtual_device();
  if (error) {
    return error;
  }
  return input_register_handler(&usb_mouse_handler);
}

static void __exit usb_mouse_exit(void) {
  input_unregister_handler(&usb_mouse_handler);
  input_unregister_device(virtual_input_dev);
  input_free_device(virtual_input_dev);
}

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");

module_init(usb_mouse_init);
module_exit(usb_mouse_exit);

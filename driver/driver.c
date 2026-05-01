#include "accel.h"
#include "util.h"

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <linux/hid.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/usb/input.h>

#define NONE_EVENT_VALUE 0

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0))
#define __cleanup_events 0
#else
#define __cleanup_events 1
#endif

static ssize_t mouse_param_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t mouse_param_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);

#define FILE_PERMISSIONS (0660)

// Manual definition since we aren't using the default <name>_show naming convention
static struct device_attribute dev_attr_acceleration_mode = __ATTR(acceleration_mode, FILE_PERMISSIONS, mouse_param_show, mouse_param_store);

static struct device_attribute dev_attr_input_cap    = __ATTR(input_cap, FILE_PERMISSIONS, mouse_param_show, mouse_param_store);
static struct device_attribute dev_attr_sensitivity  = __ATTR(sensitivity,  FILE_PERMISSIONS, mouse_param_show, mouse_param_store);
static struct device_attribute dev_attr_ratio_yx     = __ATTR(ratio_yx,     FILE_PERMISSIONS, mouse_param_show, mouse_param_store);
static struct device_attribute dev_attr_output_cap   = __ATTR(output_cap,   FILE_PERMISSIONS, mouse_param_show, mouse_param_store);
static struct device_attribute dev_attr_offset       = __ATTR(offset,       FILE_PERMISSIONS, mouse_param_show, mouse_param_store);
static struct device_attribute dev_attr_prescale     = __ATTR(prescale,     FILE_PERMISSIONS, mouse_param_show, mouse_param_store);
static struct device_attribute dev_attr_acceleration = __ATTR(acceleration, FILE_PERMISSIONS, mouse_param_show, mouse_param_store);
static struct device_attribute dev_attr_exponent     = __ATTR(exponent,     FILE_PERMISSIONS, mouse_param_show, mouse_param_store);
static struct device_attribute dev_attr_midpoint     = __ATTR(midpoint,     FILE_PERMISSIONS, mouse_param_show, mouse_param_store);
static struct device_attribute dev_attr_motivity     = __ATTR(motivity,     FILE_PERMISSIONS, mouse_param_show, mouse_param_store);
static struct device_attribute dev_attr_use_smoothing = __ATTR(use_smoothing, FILE_PERMISSIONS, mouse_param_show, mouse_param_store);

static struct device_attribute dev_attr_lut_size = __ATTR(lut_size, FILE_PERMISSIONS, mouse_param_show, mouse_param_store);
static struct device_attribute dev_attr_lut_data = __ATTR(lut_data, FILE_PERMISSIONS, mouse_param_show, mouse_param_store);

static struct device_attribute dev_attr_cc_data_aggregate = __ATTR(cc_data_aggregate, FILE_PERMISSIONS, mouse_param_show, mouse_param_store);

static struct device_attribute dev_attr_rotation_angle = __ATTR(rotation_angle, FILE_PERMISSIONS, mouse_param_show, mouse_param_store);
static struct device_attribute dev_attr_angle_snap_threshold = __ATTR(angle_snap_threshold, FILE_PERMISSIONS, mouse_param_show, mouse_param_store);
static struct device_attribute dev_attr_angle_snap_angle = __ATTR(angle_snap_angle, FILE_PERMISSIONS, mouse_param_show, mouse_param_store);


static struct attribute *mouse_attrs[] = {
    &dev_attr_acceleration_mode.attr,
    &dev_attr_input_cap.attr,
    &dev_attr_ratio_yx.attr,
    &dev_attr_output_cap.attr,
    &dev_attr_offset.attr,
    &dev_attr_prescale.attr,
    &dev_attr_acceleration.attr,
    &dev_attr_sensitivity.attr,
    &dev_attr_exponent.attr,
    &dev_attr_midpoint.attr,
    &dev_attr_motivity.attr,
    &dev_attr_use_smoothing.attr,
    &dev_attr_lut_size.attr,
    &dev_attr_lut_data.attr,
    &dev_attr_cc_data_aggregate.attr,
    &dev_attr_rotation_angle.attr,
    &dev_attr_angle_snap_threshold.attr,
    &dev_attr_angle_snap_angle.attr,
    NULL,
};

static const struct attribute_group mouse_attr_group = {
    .name = "accel_config",
    .attrs = mouse_attrs,
};

static const struct attribute_group *mouse_groups[] = {
    &mouse_attr_group,
    NULL,
};

struct mouse_state {
    int x;
    int y;
    struct accel_params *params;
};

#if __cleanup_events
static unsigned int driver_events(struct input_handle *handle, struct input_value *vals, unsigned int count) {
#else
static void driver_events(struct input_handle *handle, const struct input_value *vals, unsigned int count) {
#endif
    struct mouse_state *state = handle->private;
    struct input_dev *dev = handle->dev;
    unsigned int out_count = count;
    struct input_value *v_syn = NULL;
    struct input_value *end = vals;
    struct input_value *v;
    int error;
    bool seen_x = false;
    bool seen_y = false;

    for (v = (struct input_value *) vals; v != vals + count; v++) {
        if (v->type == EV_REL) {
            /* Find input_value for EV_REL events we're interested in and store values */
            switch (v->code) {
                case REL_X:
                    state->x = v->value;
                    seen_x = true;
                    break;
                case REL_Y:
                    state->y = v->value;
                    seen_y = true;
                    break;
                default: break;
            }
        } else if (v->type == EV_SYN && v->code == SYN_REPORT &&
                   (state->x != NONE_EVENT_VALUE || state->y != NONE_EVENT_VALUE)) {
            /* If we find an EV_SYN event, and we've seen x/y values, we store the pointer and apply acceleration next */
            v_syn = v;
            break;
        }
    }

    if (!v_syn)
        goto unchanged_return;

    int x = state->x;
    int y = state->y;
    /* If we found no values to update, return */
    if (x == NONE_EVENT_VALUE && y == NONE_EVENT_VALUE)
        goto unchanged_return;

    // Get the accel params
    struct accel_params *params = state->params;
    error = accelerate(&x, &y);
    /* Reset state */
    state->x = NONE_EVENT_VALUE;
    state->y = NONE_EVENT_VALUE;
    /* Deal with leftover EV_REL events we should take into account for the next run */
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
                default: break;
            }
        }
    }

    if (error)
        goto unchanged_return;

    /* Apply updates after we've captured events for the next run */
    for (v = (struct input_value *) vals; v != vals + count; v++) {
        if (v->type == EV_REL) {
            switch (v->code) {
                case REL_X:
                    v->value = x;
                    break;
                case REL_Y:
                    v->value = y;
                    break;
                default: break;
            }
        }
        if (end != v)
            *end = *v;
        end++;
    }

#if __cleanup_events
    /* Inject missing axes if needed */
    if (x != NONE_EVENT_VALUE && !seen_x && (unsigned int) (end - vals) < dev->max_vals) {
        end->type = EV_REL;
        end->code = REL_X;
        end->value = x;
        end++;
    }
    if (y != NONE_EVENT_VALUE && !seen_y && (unsigned int) (end - vals) < dev->max_vals) {
        end->type = EV_REL;
        end->code = REL_Y;
        end->value = y;
        end++;
    }

    /* Copy the SYN we broke on */
    if (end != v_syn) {
        *end = *v_syn;
        end++;
    }
#endif

    out_count = end - vals;
    dev->num_vals = out_count;
    /* NOTE: Technically, we can also stop iterating over `vals` when we find EV_SYN, apply acceleration,
     * then restart in a loop until we reach the end of `vals` to handle multiple EV_SYN events per batch.
     * However, that's not necessary since we can assume that all events in `vals` apply to the same moment
     * in time. */
unchanged_return:
#if __cleanup_events
    return out_count;
#else
    return;
#endif
}

static bool driver_match(struct input_handler *handler, struct input_dev *dev) {
    // Discard device if it doesn't have left button key capabilities
    if (!test_bit(EV_KEY, dev->evbit) || !test_bit(BTN_LEFT, dev->keybit) || !test_bit(BTN_MOUSE, dev->keybit))
        return false;

    // Discard device if it doesn't have relative movement capabilities
    if (!test_bit(EV_REL, dev->evbit) || !test_bit(REL_X, dev->relbit) || !test_bit(REL_Y, dev->relbit))
        return false;

    if (dev->dev.parent && dev->dev.parent->bus == &hid_bus_type) {
        struct hid_device *hdev = to_hid_device(dev->dev.parent);
        pr_info("found a possible mouse (HID) %s", hdev->name);
        return true;
    }

    // handle other non-HID devices, like virtual devices
    // NOTE: keyd actually emulates a USB device with BUS_USB:
    // https://github.com/rvaiya/keyd/blob/7c0aecb8bfd34dc8642bf4eefd2e59c89e61cec3/src/vkbd/uinput.c#L87
    if (dev->id.bustype == BUS_USB || dev->id.bustype == BUS_VIRTUAL) {
        pr_info("found a possible mouse %s", dev->name ?: "unknown");
        return true;
    }
    pr_warn("skipping incompatible device %s", dev->name);

    return false;
}

/* Same as Linux's input_register_handle but we always add the handle to the head of handlers */
static int input_register_handle_head(struct input_handle *handle) {
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

static int driver_connect(struct input_handler *handler, struct input_dev *dev, const struct input_device_id *id) {
    struct input_handle *handle;
    struct mouse_state *state;
    struct accel_params *accel_config;
    int error;

    handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
    if (!handle)
        return -ENOMEM;

    state = kzalloc(sizeof(struct mouse_state), GFP_KERNEL);
    if (!state) {
        kfree(handle);
        return -ENOMEM;
    }

    accel_config = kzalloc(sizeof(struct accel_params), GFP_KERNEL);
    if (!accel_config) {
        kfree(handle);
        kfree(state);
        return -ENOMEM;
    }
    state->params = accel_config;

    input_set_drvdata(dev, state);

    error = sysfs_create_group(&dev->dev.kobj, &mouse_attr_group);
    if (error) {
        pr_err("Failed to create sysfs group: %d\n", error);
        goto err_free_mem;
    }

    state->x = NONE_EVENT_VALUE;
    state->y = NONE_EVENT_VALUE;

    handle->private = state;
    handle->dev = input_get_device(dev);
    handle->handler = handler;
    handle->name = "yeetmouse";

    /* WARN: Instead of `input_register_handle` we use a customized version of it here.
     * This prepends the handler (like a filter) instead of appending it, making
     * it take precedence over any other input handler that'll be added. */
    error = input_register_handle_head(handle);
    if (error)
        goto err_free_mem;

    error = input_open_device(handle);
    if (error)
        goto err_unregister_handle;

    pr_info("connecting to device: %s (%s at %s)", dev_name(&dev->dev), dev->name ?: "unknown",
           dev->phys ?: "unknown");

    return 0;

err_unregister_handle:
    input_unregister_handle(handle);

err_free_mem:
    kfree(handle->private);
    kfree(handle);
    return error;
}

static void driver_disconnect(struct input_handle *handle) {
    input_close_device(handle);
    input_unregister_handle(handle);
    kfree(((struct mouse_state*)handle->private)->params);
    kfree(handle->private);
    sysfs_remove_group(&handle->dev->dev.kobj, &mouse_attr_group);
    kfree(handle);
}

static const struct input_device_id driver_ids[] = {
    {
        .flags = INPUT_DEVICE_ID_MATCH_EVBIT,
        .evbit = {BIT_MASK(EV_REL)}
    },
    {},
};

MODULE_DEVICE_TABLE(input, driver_ids);

struct input_handler driver_handler = {
    .name = "yeetmouse",
    .id_table = driver_ids,
    .events = driver_events,
    .connect = driver_connect,
    .disconnect = driver_disconnect,
    .match = driver_match
};

static ssize_t mouse_param_show(struct device *dev, struct device_attribute *attr, char *buf) {
    struct input_dev *idev = to_input_dev(dev);
    struct mouse_state *state = input_get_drvdata(idev);
    if (!state) return -ENODEV;

    struct accel_params *params = state->params;

    if (attr == &dev_attr_acceleration_mode)
        return sysfs_emit(buf, "%d\n", params->acceleration_mode);
    if (attr == &dev_attr_input_cap)
        return sysfs_emit(buf, "%lld\n", params->input_cap);
    if (attr == &dev_attr_ratio_yx)
        return sysfs_emit(buf, "%lld\n", params->ratio_yx);
    if (attr == &dev_attr_output_cap)
        return sysfs_emit(buf, "%lld\n", params->output_cap);
    if (attr == &dev_attr_offset)
        return sysfs_emit(buf, "%lld\n", params->offset);
    if (attr == &dev_attr_prescale)
        return sysfs_emit(buf, "%lld\n", params->prescale);
    if (attr == &dev_attr_acceleration)
        return sysfs_emit(buf, "%lld\n", params->acceleration);
    if (attr == &dev_attr_sensitivity)
        return sysfs_emit(buf, "%lld\n", params->sensitivity);
    if (attr == &dev_attr_exponent)
        return sysfs_emit(buf, "%lld\n", params->exponent);
    if (attr == &dev_attr_midpoint)
        return sysfs_emit(buf, "%lld\n", params->midpoint);
    if (attr == &dev_attr_motivity)
        return sysfs_emit(buf, "%lld\n", params->motivity);
    if (attr == &dev_attr_use_smoothing)
        return sysfs_emit(buf, "%d\n", params->use_smoothing);
    if (attr == &dev_attr_lut_size)
        return sysfs_emit(buf, "%lu\n", params->lut_size);
    if (attr == &dev_attr_lut_data)
        return sysfs_emit(buf, "%s\n", params->lut_data);
    if (attr == &dev_attr_cc_data_aggregate)
        return sysfs_emit(buf, "%s\n", params->cc_data_aggregate);
    if (attr == &dev_attr_rotation_angle)
        return sysfs_emit(buf, "%lld\n", params->rotation_angle);
    if (attr == &dev_attr_angle_snap_threshold)
        return sysfs_emit(buf, "%lld\n", params->angle_snap_threshold);
    if (attr == &dev_attr_angle_snap_angle)
        return sysfs_emit(buf, "%lld\n", params->angle_snap_angle);
    return -EINVAL;
}

static ssize_t mouse_param_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
    struct input_dev *idev = to_input_dev(dev);
    struct mouse_state *state = input_get_drvdata(idev);
    if (!state) return -ENODEV;

    struct accel_params *data = state->params;
    long long val;
    int ret;

    ret = kstrtoll(buf, 10, &val);
    if (ret) return ret;

    if (attr == &dev_attr_acceleration_mode)
        data->acceleration_mode = val;
    else if (attr == &dev_attr_input_cap)
        data->input_cap = val;
    else if (attr == &dev_attr_ratio_yx)
        data->ratio_yx = val;
    else if (attr == &dev_attr_output_cap)
        data->output_cap = val;
    else if (attr == &dev_attr_offset)
        data->offset = val;
    else if (attr == &dev_attr_prescale)
        data->prescale = val;
    else if (attr == &dev_attr_acceleration)
        data->acceleration = val;
    else if (attr == &dev_attr_sensitivity)
        data->sensitivity = val;
    else if (attr == &dev_attr_exponent)
        data->exponent = val;
    else if (attr == &dev_attr_midpoint)
        data->midpoint = val;
    else if (attr == &dev_attr_motivity)
        data->motivity = val;
    else if (attr == &dev_attr_use_smoothing)
        data->use_smoothing = val;
    else if (attr == &dev_attr_lut_size)
        data->lut_size = val;
    else if (attr == &dev_attr_lut_data) {
        // Call the parser
    }
    else if (attr == &dev_attr_cc_data_aggregate) {
        // nop
    }
    else if (attr == &dev_attr_rotation_angle)
        data->rotation_angle = val;
    else if (attr == &dev_attr_angle_snap_threshold)
        data->angle_snap_threshold = val;
    else if (attr == &dev_attr_angle_snap_angle)
        data->angle_snap_angle = val;

    return count;
}

static int __init yeetmouse_init(void) {
    return input_register_handler(&driver_handler);
}

static void __exit yeetmouse_exit(void) {
    input_unregister_handler(&driver_handler);
}

MODULE_DESCRIPTION("USB HID input handler applying mouse acceleration (Yeetmouse)");
MODULE_LICENSE("GPL");

module_init(yeetmouse_init);
module_exit(yeetmouse_exit);

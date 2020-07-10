#include <guiddef.h>

#include "usbip_windows.h"

#include "usbip_common.h"
#include "usbip_setupdi.h"
#include "usbip_stub.h"
#include "usbip_network.h"

#include "winra1n.h"

typedef struct {
	unsigned short	vendor, product;
	unsigned char	devno;
} usbdev_t;

typedef struct {
	int		n_usbdevs;
	usbdev_t* usbdevs;
} usbdev_list_t;

static int walker_bind(HDEVINFO dev_info, PSP_DEVINFO_DATA pdev_info_data, devno_t devno, void* ctx) {
	devno_t* pdevno = (devno_t*)ctx;

	if (devno == *pdevno) {
		/* gotcha */
		if (attach_stub_driver(devno)) {
			restart_device(dev_info, pdev_info_data);
			return -100;
		}
	}
	return 0;
}

static int walker_unbind(HDEVINFO dev_info, PSP_DEVINFO_DATA pdev_info_data, devno_t devno, void* ctx) {
	devno_t* pdevno = (devno_t*)ctx;

	if (devno == *pdevno) {
		/* gotcha */
		if (detach_stub_driver(devno)) {
			restart_device(dev_info, pdev_info_data);
			return 1;
		}
		return -2;
	}
	return 0;
}

int usbip_winra1n(int argc, char* argv[]) {
	static const struct option opts[] = {
		{ "unbind", optional_argument, NULL, 'u' },
		{ NULL,    0,                 NULL,  0  }
	};

	int ret = -3;
	int opt;

	for (;;) {
		opt = getopt_long(argc, argv, "u:", opts, NULL);

		if (opt == -1) {
			err("%s: invalid argument", __FUNCTION__);
			return -1;
		}
		else if (opt == 'u') {
			break;
		}
	}

	usbdev_list_t* usbdev_list;

	usbdev_list = usbip_list_usbdevs();
	if (usbdev_list == NULL) {
		err("%s: error binding Apple Mobile Device: USB list empty", __FUNCTION__);
		return 2;
	}

	for (int i = 0; i < usbdev_list->n_usbdevs; i++) {
		if ((usbdev_list->usbdevs + i)->vendor == 1452 && (usbdev_list->usbdevs + i)->product == 4647) {
			if (strcmp(optarg, "1")) {
				ret = traverse_usbdevs(walker_unbind, TRUE, (void*)(usbdev_list->usbdevs + i)->devno);

				switch (ret) {
				case 0:
					err("%s: error unbinding Apple Mobile Device: no such device", __FUNCTION__);
					return 1;
				case 1:
					info("%s: unbind Apple Mobile Device: complete", __FUNCTION__);
					return 0;
				default:
					err("%s: error unbinding Apple Mobile Device: failed to unbind device (helpful, I know)", __FUNCTION__);
					return 1;
				}
			}
			else {
				ret = traverse_usbdevs(walker_bind, TRUE, (void*)(usbdev_list->usbdevs + i)->devno);

				if (ret != -100) {
					err("%s: error binding Apple Mobile Device: err: %d", __FUNCTION__, ret);
				}
				else {
					info("%s: bind device Apple Mobile Device: complete", __FUNCTION__);
				}
			}
		}
	}

	switch (ret) {
	case -100:
		goto out;
	case -3:
		err("%s: error binding Apple Mobile Device: no device found", __FUNCTION__);
		break;
	default:
		goto out;
	}
out:
	return ret;
}
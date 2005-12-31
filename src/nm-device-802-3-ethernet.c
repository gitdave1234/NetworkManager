/* NetworkManager -- Network link manager
 *
 * Dan Williams <dcbw@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * (C) Copyright 2005 Red Hat, Inc.
 */

#include <glib.h>
#include <glib/gi18n.h>
#include <dbus/dbus.h>
#include <netinet/in.h>
#include <string.h>
#include <net/ethernet.h>
#include <stdlib.h>

#include "nm-device-802-3-ethernet.h"
#include "nm-device-private.h"
#include "NetworkManagerMain.h"
#include "nm-activation-request.h"
#include "NetworkManagerUtils.h"
#include "nm-utils.h"

#define NM_DEVICE_802_3_ETHERNET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), NM_TYPE_DEVICE_802_3_ETHERNET, NMDevice8023EthernetPrivate))

struct _NMDevice8023EthernetPrivate
{
	gboolean	dispose_has_run;

	struct ether_addr	hw_addr;
};

static gboolean supports_mii_carrier_detect (NMDevice8023Ethernet *dev);
static gboolean supports_ethtool_carrier_detect (NMDevice8023Ethernet *dev);


static void
nm_device_802_3_ethernet_init (NMDevice8023Ethernet * self)
{
	self->priv = NM_DEVICE_802_3_ETHERNET_GET_PRIVATE (self);
	self->priv->dispose_has_run = FALSE;

	memset (&(self->priv->hw_addr), 0, sizeof (struct ether_addr));
}


/*
 * nm_device_get_hw_address
 *
 * Get a device's hardware address
 *
 */
void
nm_device_802_3_ethernet_get_address (NMDevice8023Ethernet *self, struct ether_addr *addr)
{
	g_return_if_fail (self != NULL);
	g_return_if_fail (addr != NULL);

	memcpy (addr, &(self->priv->hw_addr), sizeof (struct ether_addr));
}


static guint32
real_discover_generic_capabilities (NMDevice *dev)
{
	NMDevice8023Ethernet *	self = NM_DEVICE_802_3_ETHERNET (dev);
	guint32		caps = NM_DEVICE_CAP_NONE;
	const char *	udi = NULL;
	char *		usb_test = NULL;
	NMData *		app_data;

	/* cipsec devices are also explicitly unsupported at this time */
	if (strstr (nm_device_get_iface (dev), "cipsec"))
		return NM_DEVICE_CAP_NONE;

	/* Ignore Ethernet-over-USB devices too for the moment (Red Hat #135722) */
	app_data = nm_device_get_app_data (dev);
	udi = nm_device_get_udi (dev);
	if (    libhal_device_property_exists (app_data->hal_ctx, udi, "usb.interface.class", NULL)
		&& (usb_test = libhal_device_get_property_string (app_data->hal_ctx, udi, "usb.interface.class", NULL)))
	{
		libhal_free_string (usb_test);
		return NM_DEVICE_CAP_NONE;
	}

	if (supports_ethtool_carrier_detect (self) || supports_mii_carrier_detect (self))
		caps |= NM_DEVICE_CAP_CARRIER_DETECT;

	return caps;
}

static gboolean
real_activation_config (NMDevice *dev, NMActRequest *req)
{
	NMData *	data;

	g_return_val_if_fail (req != NULL, FALSE);

	data = nm_act_request_get_data (req);
	g_assert (data);

	return TRUE;
}


static gboolean
real_probe_link (NMDevice *dev)
{
	NMDevice8023Ethernet *	self = NM_DEVICE_802_3_ETHERNET (dev);
	gboolean				link = FALSE;
	gchar *				contents;
	gchar *				carrier_path;
	gsize				length;
	guint32				caps;

	if (nm_device_get_removed (dev))
		return FALSE;

	carrier_path = g_strdup_printf ("/sys/class/net/%s/carrier", nm_device_get_iface (dev));
	if (g_file_get_contents (carrier_path, &contents, &length, NULL))
	{
		link = (gboolean) atoi (contents);
		g_free (contents);
	}
	g_free (carrier_path);

	/* We say that non-carrier-detect devices always have a link, because
	 * they never get auto-selected by NM.  User has to force them on us,
	 * so we just hope the user knows whether or not the cable's plugged in.
	 */
	caps = nm_device_get_capabilities (dev);
	if (!(caps & NM_DEVICE_CAP_CARRIER_DETECT))
		link = TRUE;

	return link;

}

static void
nm_device_802_3_ethernet_dispose (GObject *object)
{
	NMDevice8023Ethernet *		self = NM_DEVICE_802_3_ETHERNET (object);
	NMDevice8023EthernetClass *	klass = NM_DEVICE_802_3_ETHERNET_GET_CLASS (object);
	NMDeviceClass *			parent_class;  

	if (self->priv->dispose_has_run)
		/* If dispose did already run, return. */
		return;

	/* Make sure dispose does not run twice. */
	self->priv->dispose_has_run = TRUE;

	/* 
	 * In dispose, you are supposed to free all types referenced from this
	 * object which might themselves hold a reference to self. Generally,
	 * the most simple solution is to unref all members on which you own a 
	 * reference.
	 */

	/* Chain up to the parent class */
	parent_class = NM_DEVICE_CLASS (g_type_class_peek_parent (klass));
	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
nm_device_802_3_ethernet_finalize (GObject *object)
{
	NMDevice8023Ethernet *		self = NM_DEVICE_802_3_ETHERNET (object);
	NMDevice8023EthernetClass *	klass = NM_DEVICE_802_3_ETHERNET_GET_CLASS (object);
	NMDeviceClass *			parent_class;  

	/* Chain up to the parent class */
	parent_class = NM_DEVICE_CLASS (g_type_class_peek_parent (klass));
	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
nm_device_802_3_ethernet_class_init (NMDevice8023EthernetClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	NMDeviceClass *parent_class = NM_DEVICE_CLASS (klass);

	object_class->dispose = nm_device_802_3_ethernet_dispose;
	object_class->finalize = nm_device_802_3_ethernet_finalize;

	parent_class->activation_config = real_activation_config;
	parent_class->discover_generic_capabilities = real_discover_generic_capabilities;
	parent_class->probe_link = real_probe_link;

	g_type_class_add_private (object_class, sizeof (NMDevice8023EthernetPrivate));
}

GType
nm_device_802_3_ethernet_get_type (void)
{
	static GType type = 0;
	if (type == 0)
	{
		static const GTypeInfo info =
		{
			sizeof (NMDevice8023EthernetClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) nm_device_802_3_ethernet_class_init,
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (NMDevice8023Ethernet),
			0,      /* n_preallocs */
			(GInstanceInitFunc) nm_device_802_3_ethernet_init
		};
		type = g_type_register_static (NM_TYPE_DEVICE,
					       "NMDevice8023Ethernet",
					       &info, 0);
	}
	return type;
}


/**************************************/
/*    Ethtool capability detection    */
/**************************************/
#include <linux/sockios.h>
#include <linux/ethtool.h>

static gboolean
supports_ethtool_carrier_detect (NMDevice8023Ethernet *self)
{
	NMSock *			sk;
	struct ifreq		ifr;
	gboolean			supports_ethtool = FALSE;
	struct ethtool_cmd	edata;
	const char *		iface;

	g_return_val_if_fail (self != NULL, FALSE);

	iface = nm_device_get_iface (NM_DEVICE (self));
	if ((sk = nm_dev_sock_open (NM_DEVICE (self), DEV_GENERAL, __func__, NULL)) == NULL)
	{
		nm_warning ("cannot open socket on interface %s for ethtool detect; errno=%d",
				iface, errno);
		return FALSE;
	}

	strncpy (ifr.ifr_name, iface, sizeof(ifr.ifr_name) - 1);
	edata.cmd = ETHTOOL_GLINK;
	ifr.ifr_data = (char *) &edata;
#ifdef IOCTL_DEBUG
	nm_info ("%s: About to ETHTOOL\n", iface);
#endif
	if (ioctl (nm_dev_sock_get_fd (sk), SIOCETHTOOL, &ifr) == -1)
		goto out;

	supports_ethtool = TRUE;

out:
#ifdef IOCTL_DEBUG
	nm_info ("%s: Done with ETHTOOL\n", iface);
#endif
	nm_dev_sock_close (sk);
	return supports_ethtool;
}



/**************************************/
/*    MII capability detection        */
/**************************************/
#include <linux/mii.h>

static int
mdio_read (NMDevice8023Ethernet *self, NMSock *sk, struct ifreq *ifr, int location)
{
	struct mii_ioctl_data *mii;
	int val = -1;
	const char *	iface;

	g_return_val_if_fail (sk != NULL, -1);
	g_return_val_if_fail (ifr != NULL, -1);

	iface = nm_device_get_iface (NM_DEVICE (self));

	mii = (struct mii_ioctl_data *) &(ifr->ifr_data);
	mii->reg_num = location;

#ifdef IOCTL_DEBUG
	nm_info ("%s: About to GET MIIREG\n", iface);
#endif
	if (ioctl (nm_dev_sock_get_fd (sk), SIOCGMIIREG, ifr) >= 0)
		val = mii->val_out;
#ifdef IOCTL_DEBUG
	nm_info ("%s: Done with GET MIIREG\n", iface);
#endif

	return val;
}

static gboolean
supports_mii_carrier_detect (NMDevice8023Ethernet *self)
{
	NMSock *		sk;
	struct ifreq	ifr;
	int			bmsr;
	gboolean		supports_mii = FALSE;
	int			err;
	const char *	iface;

	g_return_val_if_fail (self != NULL, FALSE);

	iface = nm_device_get_iface (NM_DEVICE (self));
	if ((sk = nm_dev_sock_open (NM_DEVICE (self), DEV_GENERAL, __FUNCTION__, NULL)) == NULL)
	{
		nm_warning ("cannot open socket on interface %s for MII detect; errno=%d",
				iface, errno);
		return FALSE;
	}

	strncpy (ifr.ifr_name, iface, sizeof (ifr.ifr_name) - 1);
#ifdef IOCTL_DEBUG
	nm_info ("%s: About to GET MIIPHY\n", iface);
#endif
	err = ioctl (nm_dev_sock_get_fd (sk), SIOCGMIIPHY, &ifr);
#ifdef IOCTL_DEBUG
	nm_info ("%s: Done with GET MIIPHY\n", iface);
#endif
	if (err < 0)
		goto out;

	/* If we can read the BMSR register, we assume that the card supports MII link detection */
	bmsr = mdio_read (self, sk, &ifr, MII_BMSR);
	supports_mii = (bmsr != -1) ? TRUE : FALSE;

out:
	nm_dev_sock_close (sk);
	return supports_mii;	
}

#include "nm-device-802-3-ethernet.h"
#include "nm-device-private.h"
#include "nm-object-private.h"

#include "nm-device-802-3-ethernet-bindings.h"

G_DEFINE_TYPE (NMDevice8023Ethernet, nm_device_802_3_ethernet, NM_TYPE_DEVICE)

#define NM_DEVICE_802_3_ETHERNET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), NM_TYPE_DEVICE_802_3_ETHERNET, NMDevice8023EthernetPrivate))

typedef struct {
	DBusGProxy *proxy;

	char * hw_address;
	guint32 speed;
	gboolean carrier;
	gboolean carrier_valid;

	gboolean disposed;
} NMDevice8023EthernetPrivate;

enum {
	PROP_0,
	PROP_HW_ADDRESS,
	PROP_SPEED,
	PROP_CARRIER,

	LAST_PROP
};

#define DBUS_PROP_HW_ADDRESS "HwAddress"
#define DBUS_PROP_SPEED "Speed"
#define DBUS_PROP_CARRIER "Carrier"

GObject *
nm_device_802_3_ethernet_new (DBusGConnection *connection, const char *path)
{
	g_return_val_if_fail (connection != NULL, NULL);
	g_return_val_if_fail (path != NULL, NULL);

	return g_object_new (NM_TYPE_DEVICE_802_3_ETHERNET,
	                     NM_OBJECT_DBUS_CONNECTION, connection,
	                     NM_OBJECT_DBUS_PATH, path,
	                     NULL);
}

const char *
nm_device_802_3_ethernet_get_hw_address (NMDevice8023Ethernet *device)
{
	NMDevice8023EthernetPrivate *priv;

	g_return_val_if_fail (NM_IS_DEVICE_802_3_ETHERNET (device), NULL);

	priv = NM_DEVICE_802_3_ETHERNET_GET_PRIVATE (device);
	if (!priv->hw_address) {
		priv->hw_address = nm_object_get_string_property (NM_OBJECT (device),
		                                                  NM_DBUS_INTERFACE_DEVICE_WIRED,
		                                                  DBUS_PROP_HW_ADDRESS);
	}

	return priv->hw_address;
}

guint32
nm_device_802_3_ethernet_get_speed (NMDevice8023Ethernet *device)
{
	NMDevice8023EthernetPrivate *priv;

	g_return_val_if_fail (NM_IS_DEVICE_802_3_ETHERNET (device), 0);

	priv = NM_DEVICE_802_3_ETHERNET_GET_PRIVATE (device);
	if (!priv->speed) {
		priv->speed = nm_object_get_uint_property (NM_OBJECT (device),
		                                           NM_DBUS_INTERFACE_DEVICE_WIRED,
		                                           DBUS_PROP_SPEED);
	}

	return priv->speed;
}

gboolean
nm_device_802_3_ethernet_get_carrier (NMDevice8023Ethernet *device)
{
	NMDevice8023EthernetPrivate *priv;

	g_return_val_if_fail (NM_IS_DEVICE_802_3_ETHERNET (device), FALSE);

	priv = NM_DEVICE_802_3_ETHERNET_GET_PRIVATE (device);
	if (!priv->carrier_valid) {
		priv->carrier = nm_object_get_boolean_property (NM_OBJECT (device),
		                                                NM_DBUS_INTERFACE_DEVICE_WIRED,
		                                                DBUS_PROP_CARRIER);
		priv->carrier_valid = TRUE;
	}

	return priv->carrier;
}

static void
nm_device_802_3_ethernet_init (NMDevice8023Ethernet *device)
{
	NMDevice8023EthernetPrivate *priv = NM_DEVICE_802_3_ETHERNET_GET_PRIVATE (device);

	priv->disposed = FALSE;
	priv->carrier = FALSE;
	priv->carrier_valid = FALSE;
}

static void
register_for_property_changed (NMDevice8023Ethernet *device)
{
	NMDevice8023EthernetPrivate *priv = NM_DEVICE_802_3_ETHERNET_GET_PRIVATE (device);
	const NMPropertiesChangedInfo property_changed_info[] = {
		{ NM_DEVICE_802_3_ETHERNET_HW_ADDRESS, nm_object_demarshal_generic, &priv->hw_address },
		{ NM_DEVICE_802_3_ETHERNET_SPEED,      nm_object_demarshal_generic, &priv->speed },
		{ NM_DEVICE_802_3_ETHERNET_CARRIER,    nm_object_demarshal_generic, &priv->carrier },
		{ NULL },
	};

	nm_object_handle_properties_changed (NM_OBJECT (device),
	                                     priv->proxy,
	                                     property_changed_info);
}

static GObject*
constructor (GType type,
			 guint n_construct_params,
			 GObjectConstructParam *construct_params)
{
	GObject *object;
	NMDevice8023EthernetPrivate *priv;

	object = G_OBJECT_CLASS (nm_device_802_3_ethernet_parent_class)->constructor (type,
																				  n_construct_params,
																				  construct_params);
	if (!object)
		return NULL;

	priv = NM_DEVICE_802_3_ETHERNET_GET_PRIVATE (object);

	priv->proxy = dbus_g_proxy_new_for_name (nm_object_get_connection (NM_OBJECT (object)),
	                                         NM_DBUS_SERVICE,
	                                         nm_object_get_path (NM_OBJECT (object)),
	                                         NM_DBUS_INTERFACE_DEVICE_WIRED);

	register_for_property_changed (NM_DEVICE_802_3_ETHERNET (object));

	return object;
}

static void
dispose (GObject *object)
{
	NMDevice8023EthernetPrivate *priv = NM_DEVICE_802_3_ETHERNET_GET_PRIVATE (object);

	if (priv->disposed) {
		G_OBJECT_CLASS (nm_device_802_3_ethernet_parent_class)->dispose (object);
		return;
	}

	priv->disposed = TRUE;

	g_object_unref (priv->proxy);

	G_OBJECT_CLASS (nm_device_802_3_ethernet_parent_class)->dispose (object);
}

static void
finalize (GObject *object)
{
	NMDevice8023EthernetPrivate *priv = NM_DEVICE_802_3_ETHERNET_GET_PRIVATE (object);

	if (priv->hw_address)
		g_free (priv->hw_address);

	G_OBJECT_CLASS (nm_device_802_3_ethernet_parent_class)->finalize (object);
}

static void
get_property (GObject *object,
              guint prop_id,
              GValue *value,
              GParamSpec *pspec)
{
	NMDevice8023Ethernet *device = NM_DEVICE_802_3_ETHERNET (object);

	switch (prop_id) {
	case PROP_HW_ADDRESS:
		g_value_set_string (value, nm_device_802_3_ethernet_get_hw_address (device));
		break;
	case PROP_SPEED:
		g_value_set_uint (value, nm_device_802_3_ethernet_get_speed (device));
		break;
	case PROP_CARRIER:
		g_value_set_boolean (value, nm_device_802_3_ethernet_get_carrier (device));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
nm_device_802_3_ethernet_class_init (NMDevice8023EthernetClass *device_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (device_class);

	g_type_class_add_private (device_class, sizeof (NMDevice8023EthernetPrivate));

	/* virtual methods */
	object_class->constructor = constructor;
	object_class->dispose = dispose;
	object_class->finalize = finalize;
	object_class->get_property = get_property;

	/* properties */
	g_object_class_install_property
		(object_class, PROP_HW_ADDRESS,
		 g_param_spec_string (NM_DEVICE_802_3_ETHERNET_HW_ADDRESS,
						  "MAC Address",
						  "Hardware MAC address",
						  NULL,
						  G_PARAM_READABLE));

	g_object_class_install_property
		(object_class, PROP_SPEED,
		 g_param_spec_uint (NM_DEVICE_802_3_ETHERNET_SPEED,
					    "Speed",
					    "Speed",
					    0, G_MAXUINT32, 0,
					    G_PARAM_READABLE));

	g_object_class_install_property
		(object_class, PROP_CARRIER,
		 g_param_spec_boolean (NM_DEVICE_802_3_ETHERNET_CARRIER,
					    "Carrier",
					    "Carrier",
					    FALSE,
					    G_PARAM_READABLE));

}


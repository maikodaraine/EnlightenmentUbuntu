#include <Eina.h>
#include <Eldbus.h>
#include <Ecore.h>

#include "plugin.h"
#include "song.h"

typedef struct _MPRIS_Method MPRIS_Method;
typedef struct _MPRIS_Signal MPRIS_Signal;

static int _mpris_log_domain = -1;

#ifdef CRITICAL
#undef CRITICAL
#endif
#ifdef ERR
#undef ERR
#endif
#ifdef WRN
#undef WRN
#endif
#ifdef INF
#undef INF
#endif
#ifdef DBG
#undef DBG
#endif

#define CRITICAL(...) EINA_LOG_DOM_CRIT(_mpris_log_domain, __VA_ARGS__)
#define ERR(...)      EINA_LOG_DOM_ERR(_mpris_log_domain, __VA_ARGS__)
#define WRN(...)      EINA_LOG_DOM_WARN(_mpris_log_domain, __VA_ARGS__)
#define INF(...)      EINA_LOG_DOM_INFO(_mpris_log_domain, __VA_ARGS__)
#define DBG(...)      EINA_LOG_DOM_DBG(_mpris_log_domain, __VA_ARGS__)


/*
 * Capabilities and player status values conform to the MPRIS 1.0 standard:
 * http://www.mpris.org/1.0/spec.html
 */
typedef enum {
  MPRIS_CAPABILITY_CAN_GO_NEXT = 1 << 0,
  MPRIS_CAPABILITY_CAN_GO_PREV = 1 << 1,
  MPRIS_CAPABILITY_CAN_PAUSE = 1 << 2,
  MPRIS_CAPABILITY_CAN_PLAY = 1 << 3,
  MPRIS_CAPABILITY_CAN_SEEK = 1 << 4,
  MPRIS_CAPABILITY_CAN_PROVIDE_METADATA = 1 << 5,
  MPRIS_CAPABILITY_HAS_TRACKLIST = 1 << 6
} Mpris_Capabilities;

#define APPLICATION_NAME "org.mpris.enjoy"
#define PLAYER_INTERFACE_NAME "org.freedesktop.MediaPlayer"
#define ROOT_NAME "/Root" /* should really be "/", but this doesn't work correctly :( */
#define TRACKLIST_NAME "/TrackList"
#define PLAYER_NAME "/Player"

static void _mpris_signal_player_caps_change(int caps);
static void _mpris_signal_player_status_change(int playback, int shuffle, int repeat, int endless);
static void _mpris_signal_player_track_change(const Song *song);
static void _mpris_signal_tracklist_tracklist_change(int size);

static void _mpris_append_dict_entry(Eldbus_Message_Iter *array, const char *key, const char  *value_type, ...);
static Eldbus_Message *_mpris_player_next(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message *_mpris_player_previous(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message *_mpris_player_pause(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message *_mpris_player_stop(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message *_mpris_player_play(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message *_mpris_player_seek(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message *_mpris_root_identity(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message *_mpris_root_quit(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message *_mpris_root_version(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message *_mpris_player_caps_get(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message *_mpris_player_volume_set(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message *_mpris_player_volume_get(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message *_mpris_player_repeat_set(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message *_mpris_player_status_get(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message *_mpris_player_position_set(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message *_mpris_player_position_get(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message *_mpris_song_metadata_reply(const Eldbus_Message *msg, const Song *song);
static Eldbus_Message *_mpris_player_metadata_get(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message *_mpris_tracklist_current_track_get(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message *_mpris_tracklist_count(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message *_mpris_tracklist_metadata_get(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message *_mpris_tracklist_shuffle_set(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);

static void _cb_dbus_request_name(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending);

static Eldbus_Connection *conn = NULL;
static Eldbus_Service_Interface *root, *player, *tracklist;
static Eina_List *ev_handlers = NULL;

enum
{
   PLAYER_TRACK = 0,
   PLAYER_STATUS,
   PLAYER_CAPS
};

static const Eldbus_Signal mpris_player_signals[] = {
   /* Emitted whenever a new song is played; gives the song metadata */
   [PLAYER_TRACK] = { "TrackChange",  ELDBUS_ARGS({"a{sv}", ""}), 0 },
   /* Emitted whenever player's status changes */
   [PLAYER_STATUS] = { "StatusChange", ELDBUS_ARGS({"(iiii)", ""}), 0 },
   /* Emitted whenever player's capabilities changes */
   [PLAYER_CAPS] = { "CapsChange", ELDBUS_ARGS({"i", ""}), 0 },
   {  }
};

enum
{
   TRACK_LIST = 0,
};

static const Eldbus_Signal mpris_tracklist_signals[] = {
   /* Emitted whenever the tracklist changes; gives the number of items */
   [TRACK_LIST] = { "TrackListChange", ELDBUS_ARGS({"i", ""}), 0 },
   {  }
};

static const Eldbus_Method mpris_root_methods[] = {
   /* Returns a string representing the player name */
   {
    "Identity", NULL, ELDBUS_ARGS({"s", "name"}), _mpris_root_identity, 0
   },
   /* Quits the player */
   { "Quit", NULL, NULL, _mpris_root_quit, 0 },
   /* Returns a tuple containing the version of MPRIS protocol implemented */
   {
    "MprisVersion", NULL, ELDBUS_ARGS({"(qq)", ""}), _mpris_root_version, 0
   },
   { }
};

static const Eldbus_Method mpris_player_methods[] = {
   /* Goes to the next song */
   {
    "Next", NULL, NULL, _mpris_player_next, 0
   },
   /* Goes to the previous song */
   {
    "Prev", NULL, NULL, _mpris_player_previous, 0
   },
   /* Pauses the song */
   {
    "Pause", NULL, NULL, _mpris_player_pause, 0
   },
   /* Stops the song */
   {
    "Stop", NULL, NULL, _mpris_player_stop, 0
   },
   /* If playing, rewind to the beginning of the current track; else, start playing */
   {
    "Play", NULL, NULL, _mpris_player_play, 0
   },
   /* Seek the current song by given miliseconds */
   {
    "Seek", ELDBUS_ARGS({"x", "time"}), NULL, _mpris_player_seek, 0
   },
   /* Toggle the current track repeat */
   {
    "Repeat", ELDBUS_ARGS({"b", ""}), NULL, _mpris_player_repeat_set, 0
   },
   /* Return the status of the media player */
   {
    "GetStatus", NULL, ELDBUS_ARGS({"(iiii)", ""}), _mpris_player_status_get, 0
   },
   /* Gets all the metadata for the currently played element */
   {
    "GetMetadata", NULL, ELDBUS_ARGS({"a{sv}", "data"}),
    _mpris_player_metadata_get, 0
   },
   /* Returns the media player's current capabilities */
   {
    "GetCaps", NULL, ELDBUS_ARGS({"i", ""}), _mpris_player_caps_get, 0
   },
   /* Sets the volume */
   {
    "VolumeSet", ELDBUS_ARGS({"i", ""}), NULL, _mpris_player_volume_set, 0
   },
   /* Gets the current volume */
   {
    "VolumeGet", NULL, ELDBUS_ARGS({"i", ""}), _mpris_player_volume_get, 0
   },
   /* Sets the playing position (in ms) */
   {
    "PositionSet", ELDBUS_ARGS({"i", ""}), NULL, _mpris_player_position_set, 0
   },
   /* Gets the playing position (in ms) */
   {
    "PositionGet", NULL, ELDBUS_ARGS({"i", ""}), _mpris_player_position_get, 0
   },
   { }
};

static const Eldbus_Method mpris_tracklist_methods[] = {
   /* Gives all the metadata available at the given position in the track list */
   {
    "GetMetadata", ELDBUS_ARGS({"i", ""}), ELDBUS_ARGS({"a{sv}", ""}),
    _mpris_tracklist_metadata_get, 0
   },
   /* Returns the position of the current URI in the track list */
   {
    "GetCurrentTrack", NULL, ELDBUS_ARGS({"i", ""}),
    _mpris_tracklist_current_track_get, 0
   },
   /* Returns the number of elements in the track list */
   {
    "GetLength", NULL, ELDBUS_ARGS({"i", ""}), _mpris_tracklist_count, 0
   },
   /* Appends an URI to the track list */
   /*{ "AddTrack", ELDBUS_ARGS({"sb", ""}), ELDBUS_ARGS({"i", ""}), NULL, 0 },*/
   /* Removes an URL from the track list */
   /*{ "DelTrack", ELDBUS_ARGS({"i", ""}), NULL, NULL, 0 },*/
   /* Toggle playlist loop */
   /*{ "SetLoop", ELDBUS_ARGS({"b", ""}), NULL, NULL, 0 },*/
   /* Toggle playlist shuffle/random */
   {
    "SetRandom", ELDBUS_ARGS({"b", ""}), NULL, _mpris_tracklist_shuffle_set, 0
   },
   { }
};

static int
_caps_to_mpris_bits(const Enjoy_Player_Caps caps)
{
   int bits = 0;
   if (caps.can_go_next) bits |= MPRIS_CAPABILITY_CAN_GO_NEXT;
   if (caps.can_go_prev) bits |= MPRIS_CAPABILITY_CAN_GO_PREV;
   if (caps.can_pause) bits |= MPRIS_CAPABILITY_CAN_PAUSE;
   if (caps.can_play) bits |= MPRIS_CAPABILITY_CAN_PLAY;
   if (caps.can_seek) bits |= MPRIS_CAPABILITY_CAN_SEEK;
   if (caps.can_provide_metadata) bits |= MPRIS_CAPABILITY_CAN_PROVIDE_METADATA;
   if (caps.has_tracklist) bits |= MPRIS_CAPABILITY_HAS_TRACKLIST;
   return bits;
}

static Eina_Bool
_cb_player_caps_change(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
   Enjoy_Player_Caps caps = enjoy_player_caps_get();
   int bits = _caps_to_mpris_bits(caps);
   _mpris_signal_player_caps_change(bits);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_player_status_change(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
   Enjoy_Player_Status status = enjoy_player_status_get();
   _mpris_signal_player_status_change
     (status.playback, status.shuffle, status.repeat, status.endless);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_player_track_change(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
   _mpris_signal_player_track_change(enjoy_song_current_get());
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_player_tracklist_change(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
   _mpris_signal_tracklist_tracklist_change(enjoy_playlist_count());
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
mpris_enable(Enjoy_Plugin *p __UNUSED__)
{
#define EV_HANDLER(ev, func, data)              \
   ev_handlers = eina_list_append               \
     (ev_handlers, ecore_event_handler_add(ev, func, data))

   EV_HANDLER(ENJOY_EVENT_PLAYER_CAPS_CHANGE, _cb_player_caps_change, NULL);
   EV_HANDLER(ENJOY_EVENT_PLAYER_STATUS_CHANGE, _cb_player_status_change, NULL);
   EV_HANDLER(ENJOY_EVENT_PLAYER_TRACK_CHANGE, _cb_player_track_change, NULL);
   EV_HANDLER(ENJOY_EVENT_TRACKLIST_TRACKLIST_CHANGE,
              _cb_player_tracklist_change, NULL);
#undef EV_HANDLER

   eldbus_name_request(conn, APPLICATION_NAME,
                      ELDBUS_NAME_REQUEST_FLAG_DO_NOT_QUEUE,
                      _cb_dbus_request_name, NULL);
   return EINA_TRUE;
}

static Eina_Bool
mpris_disable(Enjoy_Plugin *p __UNUSED__)
{
   Ecore_Event_Handler *eh;

   if (root)
     {
        eldbus_service_object_unregister(root);
        eldbus_service_object_unregister(tracklist);
        eldbus_service_object_unregister(player);
        root = NULL;
        tracklist = NULL;
        player = NULL;
     }

   EINA_LIST_FREE(ev_handlers, eh)
     ecore_event_handler_del(eh);

   return EINA_TRUE;
}

static const Enjoy_Plugin_Api api = {
  ENJOY_PLUGIN_API_VERSION,
  mpris_enable,
  mpris_disable
};

static Eina_Bool
mpris_init(void)
{
   if (_mpris_log_domain < 0)
     {
        _mpris_log_domain = eina_log_domain_register
          ("enjoy-mpris", EINA_COLOR_LIGHTCYAN);
        if (_mpris_log_domain < 0)
          {
             EINA_LOG_CRIT("Could not register log domain 'enjoy-mpris'");
             return EINA_FALSE;
          }
     }

   if (!ENJOY_ABI_CHECK())
     {
        ERR("ABI versions differ: enjoy=%u, mpris=%u",
            enjoy_abi_version(), ENJOY_ABI_VERSION);
        goto error;
     }

   if (conn) return EINA_TRUE;

   eldbus_init();
   conn = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SESSION);
   if (!conn)
     {
        ERR("Could not get DBus session bus");
        goto error;
     }

   enjoy_plugin_register("listener/mpris", &api, ENJOY_PLUGIN_PRIORITY_HIGH);

   return EINA_TRUE;

 error:
   eina_log_domain_unregister(_mpris_log_domain);
   _mpris_log_domain = -1;
   return EINA_FALSE;
}

void
mpris_shutdown(void)
{
   if (!conn) return;

   eldbus_connection_unref(conn);
   eldbus_shutdown();
   conn = NULL;

   if (_mpris_log_domain >= 0)
     {
        eina_log_domain_unregister(_mpris_log_domain);
        _mpris_log_domain = -1;
     }
}

static const Eldbus_Service_Interface_Desc root_desc = {
   PLAYER_INTERFACE_NAME, mpris_root_methods
};

static const Eldbus_Service_Interface_Desc player_desc = {
   PLAYER_INTERFACE_NAME, mpris_player_methods, mpris_player_signals
};

static const Eldbus_Service_Interface_Desc tracklist_desc = {
   PLAYER_INTERFACE_NAME, mpris_tracklist_methods, mpris_tracklist_signals
};

static void
_cb_dbus_request_name(void *data __UNUSED__, const Eldbus_Message *msg, Eldbus_Pending *pending __UNUSED__)
{
   const char *error_name, *error_txt;
   unsigned flag;

   if (eldbus_message_error_get(msg, &error_name, &error_txt))
     {
        ERR("Error %s %s", error_name, error_txt);
        return;
     }

   if (!eldbus_message_arguments_get(msg, "u", &flag))
     {
        ERR("Error getting arguments.");
        return;
     }

   if (flag != ELDBUS_NAME_REQUEST_REPLY_PRIMARY_OWNER)
     {
        ERR("Bus name in use by another application.");
        return;
     }

   root = eldbus_service_interface_register(conn, ROOT_NAME, &root_desc);
   player = eldbus_service_interface_register(conn, PLAYER_NAME, &player_desc);
   tracklist = eldbus_service_interface_register(conn, TRACKLIST_NAME,
                                                &tracklist_desc);
}

static void
_mpris_append_dict_entry(Eldbus_Message_Iter *array, const char *key,
                         const char  *value_type, ...)
{
   Eldbus_Message_Iter *dict, *val;
   va_list ap;

   va_start(ap, value_type);
   eldbus_message_iter_arguments_append(array, "{sv}", &dict);
   eldbus_message_iter_basic_append(dict, 's', key);
   val = eldbus_message_iter_container_new(dict, 'v', value_type);
   eldbus_message_iter_arguments_vappend(val, value_type, ap);
   eldbus_message_iter_container_close(dict, val);
   eldbus_message_iter_container_close(array, dict);
   va_end(ap);
}

static void
_mpris_message_fill_song_metadata(Eldbus_Message *msg, const Song *song)
{
   Eldbus_Message_Iter *array, *main_iter;

   if (!song) return;

   /*
     Other possible metadata:
     location s		time u
     mtime u		comment s
     rating u		year u
     date u		arturl s
     genre s		mpris:length u
     trackno s
   */

   main_iter = eldbus_message_iter_get(msg);
   eldbus_message_iter_arguments_append(main_iter, "a{sv}", &array);

   if (song->title)
     _mpris_append_dict_entry(array, "title", "s", song->title);
   if (song->flags.fetched_album && song->album)
     _mpris_append_dict_entry(array, "album", "s", song->album);
   if (song->flags.fetched_artist && song->artist)
     _mpris_append_dict_entry(array, "artist", "s", song->artist);
   if (song->flags.fetched_genre && song->genre)
     _mpris_append_dict_entry(array, "genre", "s", song->genre);
   _mpris_append_dict_entry(array, "rating", "u", song->rating);
   _mpris_append_dict_entry(array, "length", "u", song->length);
   _mpris_append_dict_entry(array, "enjoy:playcount", "i", song->playcnt);
   _mpris_append_dict_entry(array, "enjoy:filesize", "i", song->size);

   eldbus_message_iter_container_close(main_iter, array);
}

void
_mpris_signal_player_caps_change(int caps)
{
   static int old_caps = 0;
   if (caps != old_caps)
     {
        int32_t caps32 = caps;
        eldbus_service_signal_emit(player, PLAYER_CAPS, caps32);
        old_caps = caps;
     }
}

static void
_mpris_signal_player_status_change(int playback, int shuffle, int repeat, int endless)
{
   Eldbus_Message *sig;
   Eldbus_Message_Iter *st, *main_iter;
   static int old_playback = 0, old_shuffle = 0, old_repeat = 0, old_endless = 0;

   if (old_playback == playback && old_shuffle == shuffle &&
       old_repeat == repeat && old_endless == endless) return;
   old_playback = playback;
   old_shuffle = shuffle;
   old_repeat = repeat;
   old_endless = endless;

   sig = eldbus_service_signal_new(player, PLAYER_STATUS);
   if (!sig) return;

   main_iter = eldbus_message_iter_get(sig);
   eldbus_message_iter_arguments_append(main_iter, "(iiii)", &st);
   eldbus_message_iter_basic_append(st, 'i', playback);
   eldbus_message_iter_basic_append(st, 'i', shuffle);
   eldbus_message_iter_basic_append(st, 'i', repeat);
   eldbus_message_iter_basic_append(st, 'i', endless);
   eldbus_message_iter_container_close(main_iter, st);

   eldbus_service_signal_send(player, sig);
}

static void
_mpris_signal_player_track_change(const Song *song)
{
   static const void *old_song = NULL;
   if (old_song != song)
     {
        Eldbus_Message *sig = eldbus_service_signal_new(player, PLAYER_TRACK);
        if (!sig) return;
        _mpris_message_fill_song_metadata(sig, song);
        eldbus_service_signal_send(player, sig);
        old_song = song;
     }
}

static void
_mpris_signal_tracklist_tracklist_change(int size)
{
   int32_t size32 = size;
   eldbus_service_signal_emit(tracklist, TRACK_LIST, size32);
}

static Eldbus_Message *
_mpris_player_next(const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg)
{
   enjoy_control_next();
   return eldbus_message_method_return_new(msg);
}

static Eldbus_Message *
_mpris_player_previous(const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg)
{
   enjoy_control_previous();
   return eldbus_message_method_return_new(msg);
}

static Eldbus_Message *
_mpris_player_pause(const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg)
{
   enjoy_control_pause();
   return eldbus_message_method_return_new(msg);
}

static Eldbus_Message *
_mpris_player_stop(const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg)
{
   enjoy_control_stop();
   return eldbus_message_method_return_new(msg);
}

static Eldbus_Message *
_mpris_player_play(const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg)
{
   Enjoy_Player_Status status = enjoy_player_status_get();
   if (!status.playback)
     enjoy_position_set(0);
   enjoy_control_play();
   return eldbus_message_method_return_new(msg);
}

static Eldbus_Message *
_mpris_player_seek(const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg)
{
   int64_t position;
   if (!eldbus_message_arguments_get(msg, "x", &position))
     goto end;
   enjoy_control_seek(position);
end:
   return eldbus_message_method_return_new(msg);
}

static Eldbus_Message *
_mpris_root_identity(const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg)
{
   const char *identity = PACKAGE_STRING;
   Eldbus_Message *reply = eldbus_message_method_return_new(msg);
   eldbus_message_arguments_append(reply, "s", identity);
   return reply;
}

static Eldbus_Message *
_mpris_root_quit(const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg)
{
   enjoy_quit();
   return eldbus_message_method_return_new(msg);
}

static Eldbus_Message *
_mpris_root_version(const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg)
{
   Eldbus_Message *reply = eldbus_message_method_return_new(msg);
   Eldbus_Message_Iter *main_iter, *s;
   uint16_t v1 = 1, v2 = 0;

   main_iter = eldbus_message_iter_get(reply);
   eldbus_message_iter_arguments_append(main_iter, "(qq)", &s);
   eldbus_message_iter_arguments_append(s, "qq", v1, v2);
   eldbus_message_iter_container_close(main_iter, s);
   return reply;
}

static Eldbus_Message *
_mpris_player_caps_get(const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg)
{
   Eldbus_Message *reply = eldbus_message_method_return_new(msg);
   int32_t bits = _caps_to_mpris_bits(enjoy_player_caps_get());
   eldbus_message_arguments_append(reply, "i", bits);
   return reply;
}

static Eldbus_Message *
_mpris_player_volume_set(const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg)
{
   int volume;

   if (!eldbus_message_arguments_get(msg, "i", &volume))
     goto end;
   if (volume > 100)
     volume = 100;
   else if (volume < 0)
     volume = 0;
   enjoy_volume_set(volume);
end:
   return eldbus_message_method_return_new(msg);
}

static Eldbus_Message *
_mpris_player_volume_get(const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg)
{
   Eldbus_Message *reply = eldbus_message_method_return_new(msg);
   int32_t vol = enjoy_volume_get();
   eldbus_message_arguments_append(reply, "i", vol);
   return reply;
}

static Eldbus_Message *
_mpris_player_repeat_set(const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg)
{
   Eina_Bool repeat;
   if (!eldbus_message_arguments_get(msg, "b", &repeat))
     goto end;
   enjoy_control_loop_set(repeat);
end:
   return eldbus_message_method_return_new(msg);
}

static Eldbus_Message *
_mpris_player_status_get(const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg)
{
   Eldbus_Message *reply = eldbus_message_method_return_new(msg);
   Enjoy_Player_Status status = enjoy_player_status_get();
   Eldbus_Message_Iter *main_iter, *st;
   int32_t p, s, r, e;

   p = status.playback;
   s = status.shuffle;
   r = status.repeat;
   e = status.endless;

   main_iter = eldbus_message_iter_get(reply);
   eldbus_message_iter_arguments_append(main_iter, "(iiii)", &st);
   eldbus_message_iter_arguments_append(st, "iiii", p, s, r, e);
   eldbus_message_iter_container_close(main_iter, st);

   return reply;
}

static Eldbus_Message *
_mpris_player_position_set(const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg)
{
   int position;
   if (!eldbus_message_arguments_get(msg, "i", &position))
     goto end;
   enjoy_position_set(position);
end:
   return eldbus_message_method_return_new(msg);
}

static Eldbus_Message *
_mpris_player_position_get(const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg)
{
   Eldbus_Message *reply = eldbus_message_method_return_new(msg);
   int32_t pos = enjoy_position_get();
   eldbus_message_arguments_append(reply, "i", pos);
   return reply;
}

static Eldbus_Message *
_mpris_song_metadata_reply(const Eldbus_Message *msg, const Song *song)
{
   Eldbus_Message *reply = eldbus_message_method_return_new(msg);
   _mpris_message_fill_song_metadata(reply, song);
   return reply;
}

static Eldbus_Message *
_mpris_player_metadata_get(const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg)
{
   return _mpris_song_metadata_reply(msg, enjoy_song_current_get());
}

static Eldbus_Message *
_mpris_tracklist_current_track_get(const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg)
{
   Eldbus_Message *reply = eldbus_message_method_return_new(msg);
   int32_t pos = enjoy_playlist_current_position_get();
   eldbus_message_arguments_append(reply, "i", pos);
   return reply;
}

static Eldbus_Message *
_mpris_tracklist_count(const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg)
{
   Eldbus_Message *reply = eldbus_message_method_return_new(msg);
   int32_t count = enjoy_playlist_count();
   eldbus_message_arguments_append(reply, "i", count);
   return reply;
}

static Eldbus_Message *
_mpris_tracklist_metadata_get(const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg)
{
   Eldbus_Message *reply;
   const Song *song;
   int position;
   if (!eldbus_message_arguments_get(msg, "i", &position))
     return NULL;
   song = enjoy_playlist_song_position_get(position);
   reply = _mpris_song_metadata_reply(msg, song);
   return reply;
}

static Eldbus_Message *
_mpris_tracklist_shuffle_set(const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg)
{
   Eina_Bool param;
   if (!eldbus_message_arguments_get(msg, "b", &param))
     goto end;
   enjoy_control_shuffle_set(param);
end:
   return eldbus_message_method_return_new(msg);
}

EINA_MODULE_INIT(mpris_init);
EINA_MODULE_SHUTDOWN(mpris_shutdown);
